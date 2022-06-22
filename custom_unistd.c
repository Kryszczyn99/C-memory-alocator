//CUSTOM_UNISTD to pliki zamieszczone do projektu od Pana Jaworskiego

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>


#define PAGE_SIZE       4096    // D�ugo�� strony w bajtach
#define PAGE_FENCE      1       // Liczba stron na jeden p�otek
#define PAGES_AVAILABLE 16384   // Liczba stron dost�pnych dla sterty
#define PAGES_TOTAL     (PAGES_AVAILABLE + 2 * PAGE_FENCE)

uint8_t memory[PAGE_SIZE * PAGES_TOTAL] __attribute__((aligned(PAGE_SIZE)));

struct memory_fence_t {
    uint8_t first_page[PAGE_SIZE];
    uint8_t last_page[PAGE_SIZE];
};

struct mm_struct {
    intptr_t start_brk;
    intptr_t brk;
    
    // Poni�sze pola nie nale�� do standardowej struktury mm_struct
    struct memory_fence_t fence;
    intptr_t start_mmap;
} mm;

void __attribute__((constructor)) memory_init(void)
{
    //
    // Inicjuj testy
    setvbuf(stdout, NULL, _IONBF, 0); 
    srand(time(NULL));
    assert(sizeof(intptr_t) == sizeof(void*));
    
    /*
     * Architektura przestrzeni dynamicznej dla sterty, z p�otkami pami�ci:
     * 
     *  |<-   PAGES_AVAILABLE            ->|
     * ......................................
     * FppppppppppppppppppppppppppppppppppppL
     * 
     * F - p�otek pocz�tku
     * L - p�otek ko�ca
     * p - strona do u�ycia (liczba stron nie jest znana)
     */
    
    //
    // Inicjuj p�otki
    for (int i = 0; i < PAGE_SIZE; i++) {
        mm.fence.first_page[i] = rand();
        mm.fence.last_page[i] = rand();
    }
    
    //
    // Ustaw p�otki
    memcpy(memory, mm.fence.first_page, PAGE_SIZE);
    memcpy(memory + (PAGE_FENCE + PAGES_AVAILABLE) * PAGE_SIZE, mm.fence.last_page, PAGE_SIZE);

    //
    // Inicjuj struktur� opisuj�c� pami�� procesu (symulacj� tej struktury)
    mm.start_brk = (intptr_t)(memory + PAGE_SIZE);
    mm.brk = (intptr_t)(memory + PAGE_SIZE);
    mm.start_mmap = (intptr_t)(memory + (PAGE_FENCE + PAGES_AVAILABLE) * PAGE_SIZE);
    
    assert(mm.start_mmap - mm.start_brk == PAGES_AVAILABLE * PAGE_SIZE);
} 

void __attribute__((destructor)) memory_check(void)
{
    //
    // Sprawd� p�otki
    int first = memcmp(memory, mm.fence.first_page, PAGE_SIZE);
    int last = memcmp(memory + (PAGE_FENCE + PAGES_AVAILABLE) * PAGE_SIZE, mm.fence.last_page, PAGE_SIZE);
    
    printf("\n### Stan p�otk�w przestrzeni sterty:\n");
    printf("    P�otek pocz�tku: [%s]\n", first == 0 ? "poprawny" : "USZKODZONY");
    printf("    P�otek ko�ca...: [%s]\n", last == 0 ? "poprawny" : "USZKODZONY");

    printf("### Podsumowanie: \n");
        printf("    Ca�kowita przestrzeni pami�ci....: %lu bajt�w\n", mm.start_mmap - mm.start_brk);
        printf("    Pami�� zarezerwowana przez sbrk(): %lu bajt�w\n", mm.brk - mm.start_brk);
    
    //if (first || last) {
        printf("Naci�nij ENTER...");
        fgetc(stdin);
    //}
}

//
//
//


void* custom_sbrk(intptr_t delta)
{
    intptr_t current_brk = mm.brk;
    if (mm.start_brk + delta < 0) {
        errno = 0;
        return (void*)current_brk;
    }
    
    if (mm.brk + delta >= mm.start_mmap) {
        errno = ENOMEM;
        return (void*)-1;
    }
    
    mm.brk += delta;
    return (void*)current_brk;
}
