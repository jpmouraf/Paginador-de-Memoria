#include "pager.h"
#include "mmu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>

#define PAGE_SIZE (sysconf(_SC_PAGESIZE))

typedef struct {
    int valid;
    int on_disk;
    int frame_id;
    int block_id;
    int present;
    int prot;
    int dirty; 
} Page;

typedef struct {
    pid_t pid;
    Page *pages;
    int num_pages;
} Process;

typedef struct {
    int is_free;
    pid_t owner_pid;
    int owner_page_idx;
} Frame;

Process *processes = NULL;
int num_processes = 0;
int max_processes = 0;

Frame *frame_table = NULL;
int n_frames_global;
int clock_hand = 0;

int *disk_blocks = NULL;
int n_blocks_global;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

Process* get_process(pid_t pid) {
    for (int i = 0; i < num_processes; i++) {
        if (processes[i].pid == pid) return &processes[i];
    }
    return NULL;
}

void pager_init(int nframes, int nblocks) {
    pthread_mutex_lock(&lock);

    n_frames_global = nframes;
    n_blocks_global = nblocks;

    frame_table = malloc(nframes * sizeof(Frame));
    for (int i = 0; i < nframes; i++) {
        frame_table[i].is_free = 1;
        frame_table[i].owner_pid = -1;
        frame_table[i].owner_page_idx = -1;
    }

    disk_blocks = malloc(nblocks * sizeof(int));
    memset(disk_blocks, 0, nblocks * sizeof(int));

    max_processes = 10;
    processes = malloc(max_processes * sizeof(Process));
    num_processes = 0;

    pthread_mutex_unlock(&lock);
}

void pager_create(pid_t pid) {
    pthread_mutex_lock(&lock);

    if (num_processes >= max_processes) {
        max_processes *= 2;
        processes = realloc(processes, max_processes * sizeof(Process));
    }

    Process *p = &processes[num_processes++];
    p->pid = pid;
    p->num_pages = 0;
    p->pages = NULL;

    pthread_mutex_unlock(&lock);
}

void pager_destroy(pid_t pid) {
    pthread_mutex_lock(&lock);

    Process *p = get_process(pid);
    if (!p) {
        pthread_mutex_unlock(&lock);
        return;
    }

    for (int i = 0; i < p->num_pages; i++) {
        if (p->pages[i].block_id != -1) {
            disk_blocks[p->pages[i].block_id] = 0;
        }
        if (p->pages[i].present && p->pages[i].frame_id != -1) {
            frame_table[p->pages[i].frame_id].is_free = 1;
            frame_table[p->pages[i].frame_id].owner_pid = -1;
            frame_table[p->pages[i].frame_id].owner_page_idx = -1;
        }
    }

    free(p->pages);

    for (int i = 0; i < num_processes; i++) {
        if (processes[i].pid == pid) {
            if (i != num_processes - 1) {
                processes[i] = processes[num_processes - 1];
            }
            num_processes--;
            break;
        }
    }

    pthread_mutex_unlock(&lock);
}

void *pager_extend(pid_t pid) {
    pthread_mutex_lock(&lock);

    int block_id = -1;
    for (int i = 0; i < n_blocks_global; i++) {
        if (disk_blocks[i] == 0) {
            block_id = i;
            disk_blocks[i] = 1;
            break;
        }
    }

    if (block_id == -1) {
        pthread_mutex_unlock(&lock);
        errno = ENOSPC;
        return NULL;
    }

    Process *p = get_process(pid);

    p->pages = realloc(p->pages, (p->num_pages + 1) * sizeof(Page));
    Page *pg = &p->pages[p->num_pages];

    pg->valid = 1;
    pg->present = 0;
    pg->frame_id = -1;
    pg->block_id = block_id;
    pg->on_disk = 0;
    pg->prot = PROT_READ;
    pg->dirty = 0;        

    void *vaddr = (void *)(UVM_BASEADDR + p->num_pages * PAGE_SIZE);
    p->num_pages++;

    pthread_mutex_unlock(&lock);
    return vaddr;
}

void pager_fault(pid_t pid, void *addr) {
    pthread_mutex_lock(&lock);

    Process *p = get_process(pid);
    if (!p) {
        pthread_mutex_unlock(&lock);
        return;
    }

    intptr_t page_addr = ((intptr_t)addr / PAGE_SIZE) * PAGE_SIZE;
    int page_idx = (page_addr - UVM_BASEADDR) / PAGE_SIZE;

    if (page_idx < 0 || page_idx >= p->num_pages) {
        pthread_mutex_unlock(&lock);
        return;
    }

    Page *pg = &p->pages[page_idx];

    // CASO 1: Página já presente
    if (pg->present) {
        if (pg->prot == PROT_NONE) {
            pg->prot = PROT_READ;
            mmu_chprot(pid, (void*)page_addr, PROT_READ);
        } else {
            pg->prot = PROT_READ | PROT_WRITE;
            pg->dirty = 1;
            mmu_chprot(pid, (void*)page_addr, PROT_READ | PROT_WRITE);
        }
        pthread_mutex_unlock(&lock);
        return;
    }

    // CASO 2: Página não presente, busca Quadro Livre
    int frame = -1;
    for (int i = 0; i < n_frames_global; i++) {
        if (frame_table[i].is_free) {
            frame = i;
            break;
        }
    }

    // Algoritmo de Segunda Chance 
    if (frame == -1) {
        while (1) {
            Frame *f = &frame_table[clock_hand];
            Process *vict_proc = get_process(f->owner_pid);
            Page *vict_page = &vict_proc->pages[f->owner_page_idx];
            intptr_t vict_vaddr = UVM_BASEADDR + f->owner_page_idx * PAGE_SIZE;

            if (vict_page->prot != PROT_NONE) {
                vict_page->prot = PROT_NONE;
                mmu_chprot(vict_proc->pid, (void*)vict_vaddr, PROT_NONE);
                clock_hand = (clock_hand + 1) % n_frames_global;
            } else {
                mmu_nonresident(vict_proc->pid, (void*)vict_vaddr);
                
                if (vict_page->dirty) {
                    mmu_disk_write(clock_hand, vict_page->block_id);
                    vict_page->on_disk = 1;
                }
                
                vict_page->present = 0;
                vict_page->frame_id = -1;

                frame = clock_hand;
                clock_hand = (clock_hand + 1) % n_frames_global;
                break;
            }
        }
    }

    if (pg->on_disk) {
        mmu_disk_read(pg->block_id, frame);
    } else {
        mmu_zero_fill(frame);
    }

    frame_table[frame].is_free = 0;
    frame_table[frame].owner_pid = pid;
    frame_table[frame].owner_page_idx = page_idx;

    pg->present = 1;
    pg->frame_id = frame;
    pg->prot = PROT_READ; 
    pg->dirty = 0; 

    mmu_resident(pid, (void*)page_addr, frame, pg->prot);

    pthread_mutex_unlock(&lock);
}

int pager_syslog(pid_t pid, void *addr, size_t len) {
    pthread_mutex_lock(&lock);

    Process *p = get_process(pid);
    if (!p) {
        pthread_mutex_unlock(&lock);
        errno = EINVAL;
        return -1;
    }

    unsigned char *buf = malloc(len);
    if (!buf) {
        pthread_mutex_unlock(&lock);
        if (len == 0) return 0; 
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        intptr_t curr = (intptr_t)addr + i;
        int page_idx = (curr - UVM_BASEADDR) / PAGE_SIZE;
        int offset = (curr - UVM_BASEADDR) % PAGE_SIZE;

        if (page_idx < 0 || page_idx >= p->num_pages) {
            free(buf);
            pthread_mutex_unlock(&lock);
            errno = EINVAL;
            return -1;
        }

        Page *pg = &p->pages[page_idx];

        if (!pg->present) {
            pthread_mutex_unlock(&lock);
            pager_fault(pid, (void*)(UVM_BASEADDR + page_idx * PAGE_SIZE));
            pthread_mutex_lock(&lock);
            p = get_process(pid);
            if(!p) { free(buf); pthread_mutex_unlock(&lock); return -1; }
            pg = &p->pages[page_idx];
        }

        if (pg->present && pg->prot == PROT_NONE) {
            pg->prot = PROT_READ;
            mmu_chprot(pid, (void*)(UVM_BASEADDR + page_idx * PAGE_SIZE), PROT_READ);
        }

        buf[i] = pmem[pg->frame_id * PAGE_SIZE + offset];
    }

    if (len > 0) {
        for (size_t i = 0; i < len; i++) {
            printf("%02x", buf[i]);
        }
        printf("\n");
    }

    free(buf);
    pthread_mutex_unlock(&lock);
    return 0;
}