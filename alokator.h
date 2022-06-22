#ifndef _ALOKATOR_H
#define _ALOKATOR_H

#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "custom_unistd.h"


pthread_mutex_t watek;
pthread_mutexattr_t atrybut;

enum pointer_type_t
{
    pointer_null,
    pointer_out_of_heap,
    pointer_control_block,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};



struct zmienna_plotek
{
  int otwierajacy[8];
  int zamykajacy[8];

}plotek;

struct mblock
{
  struct mblock *next_block;//wskaznik na nastepny blok pamieci
  struct mblock *prev_block;//wskaznik na poprzedni blok pamieci

  char* filename;//nazwa pliku
  int fileline;//numer linii
  int size;//rozmiar bloku
  int locked;//czy jest zajety czy nie 
  int crc;//suma kontrolna

};

struct dane_pamieci
{
  intptr_t start_brk;//poczatek pamieci
  intptr_t brk;//koniec pamieci
  size_t stos_uzywane_miejsce;
  size_t stos_najwiekszy_uzywany;
  uint64_t stos_ilosc_uzywanych_blokow;
  size_t stos_wolne_miejsce;
  size_t stos_najwiekszy_wolny;
  uint64_t stos_wolne_bloki;
  int suma_kontrolna;

}stos;

void init_start_brk();
void init_brk();
void aktualizuj_crc (struct mblock *p);
void aktualizuj_crc_stosu();
void reset_stosu();
void aktualizuj_wszystkie_crc(struct mblock *p);
void *wypeln_zerami(void *area,int size);
struct mblock* ustaw_plotki(struct mblock* p, size_t count);
void wypeln_plotki_wartosciami();

struct mblock* znajdz(size_t count, struct mblock *p);
void init_stos_malloc_debug_null(struct mblock* p,int fileline,char* filename);
struct mblock* init_wskaznik_malloc_debug_null(struct mblock* p,int fileline,char* filename,size_t count);
void* malloc_debug_when_null(size_t count, int fileline, const char* filename);
struct mblock* malloc_debug_when_not_null(struct mblock* p,size_t count,int fileline,char* filename);

void init_stos_mal_aligned_null(int fileline,char* filename,struct mblock* p);
struct mblock* init_wskaznik_mal_aligned_null(size_t count,int fileline,char* filename,struct mblock* prev_block,struct mblock* p);
void* malloc_aligned_when_null(size_t count,int fileline,const char* filename);
struct mblock* init_wskaznik_aligned_when_not_null(struct mblock* pointer,int fileline,char* filename,size_t count,struct mblock* p);
struct mblock* znajdz2(size_t count, struct mblock *p);
void wyswietl_dane_stosu();
void wyswietl_bloki();
int porownaj_bloki();
int porownaj_crc();
int porownaj_wskazniki(struct mblock* p);
int porownywanie_danych(void* p1,void* p2,int len);
int porownaj_plotki(struct mblock* p);

int heap_setup(void);

void* heap_malloc(size_t count);
void* heap_calloc(size_t number, size_t size);
void heap_free(void* memblock);
void* heap_realloc(void* memblock, size_t size);

void* heap_malloc_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename);
void* heap_realloc_debug(void* memblock, size_t size, int fileline, const char* filename);

void* heap_malloc_aligned(size_t count);
void* heap_calloc_aligned(size_t number, size_t size);
void* heap_realloc_aligned(void* memblock, size_t size);

void* heap_malloc_aligned_debug(size_t count, int fileline, const char* filename);
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline, const char* filename);
void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline, const char* filename);

size_t heap_get_used_space(void);
size_t heap_get_largest_used_block_size(void);
uint64_t heap_get_used_blocks_count(void);
size_t heap_get_free_space(void);
size_t heap_get_largest_free_area(void);
uint64_t heap_get_free_gaps_count(void);

enum pointer_type_t get_pointer_type(const void* pointer);
void* heap_get_data_block_start(const void* pointer);
size_t heap_get_block_size(const void* memblock);

void heap_dump_debug_information(void);
int heap_validate(void);



#endif
