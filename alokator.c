#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "alokator.h"

#define WORD_SIZE sizeof(void *)
//STRONA to 4KB -> 4*1024 
#define PAGE_SIZE 4096

#define SIZE_OF_FENCE (sizeof(plotek.otwierajacy))
#define SIZE_OF_MEMORY_BLOCK (sizeof(struct mblock))

//inicjuje pola start_brk
void init_start_brk()
{
  struct mblock* brk=(struct mblock*)stos.brk;
  struct mblock* s_brk=(struct mblock*)stos.start_brk;
  //ustawienie nastepnego bloku jako brk
  s_brk->next_block=brk;
  //nic nie ma przed naszym start_brk nigdy
  s_brk->prev_block=NULL;
  s_brk->size=0;
  s_brk->locked=1;
}

//inicjuje pola brk
void init_brk()
{
  struct mblock* brk=(struct mblock*)stos.brk;
  struct mblock* s_brk=(struct mblock*)stos.start_brk;
  //ustawienie nastepnego bloku na NULL, nigdy nie ma nic za start_brk
  brk->next_block=NULL;
  //ustawienie poprzedniego bloku na start_brk
  brk->prev_block=s_brk;
  brk->size=0;
  brk->locked=1;  
}

//inicjuje pola stosu w wartosci statystyczne
void init_stos()
{
  pthread_mutex_lock(&watek);
  stos.stos_uzywane_miejsce=heap_get_used_space();
  stos.stos_najwiekszy_uzywany=heap_get_largest_used_block_size();
  stos.stos_ilosc_uzywanych_blokow=heap_get_used_blocks_count();
  stos.stos_wolne_miejsce=heap_get_free_space();
  stos.stos_najwiekszy_wolny=heap_get_largest_free_area();
  stos.stos_wolne_bloki=heap_get_free_gaps_count();
  pthread_mutex_unlock(&watek);
}

//odswieza dane wszystkich crc (poprzedni,nastepny,obecny blok)
void aktualizuj_wszystkie_crc(struct mblock *p)
{
  if(p==NULL) return;
  aktualizuj_crc(p->next_block);
  aktualizuj_crc(p->prev_block);
  aktualizuj_crc(p);
}

//funkcja aktualizuje konkretne CRC
void aktualizuj_crc (struct mblock *p)
{
  if(p==NULL) return;
  pthread_mutex_lock(&watek);
  int size=SIZE_OF_MEMORY_BLOCK,suma=0,i=0;
  p->crc=0;
  while(1)
  {
    if(i==size) break;
    int value=*((char*)p+i);
    suma=suma+value;
    i++;
  }
  p->crc=suma;
  pthread_mutex_unlock(&watek);
  aktualizuj_crc_stosu();
}

//funkcja ktora wypelnia pole pamieci o rozmiarze (size) samymi zerami
void *wypeln_zerami(void *area,int size)
{
  int i=0;
  unsigned char* text=area;
  while(1)
  {
    if(i==size) break;
    *(text+i)=0;
    i++;
  }
  return area;
}

//funkcja ustawiajace plotki/ proste kopiowanie danych do pointera z danych struktury plotek
struct mblock* ustaw_plotki(struct mblock* p, size_t count)
{

  //kopiowanie plotka_otwierajacy do wskaznika p
  intptr_t dest1=(intptr_t)p;
  dest1=dest1+SIZE_OF_MEMORY_BLOCK;
  char* t1=(char*)dest1;
  char* t2=(char*)plotek.otwierajacy;
  int i=0;
  while(1)
  {
    if(i==SIZE_OF_FENCE) break;
    *(t1+i)=*(t2+i);
    i++;
  }

  //kopiowanie plotka_zamykajacy do wskaznika p
  intptr_t dest2=(intptr_t)p;
  dest2=dest2+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE+count;
  char* t3=(char*)dest2;
  char* t4=(char*)plotek.zamykajacy;
  i=0;
  while(1)
  {
    if(i==SIZE_OF_FENCE) break;
    *(t3+i)=*(t4+i);
    i++;
  }
  return p;
}

//funkcja na zasadzie aktualizacji crc tylko do stosu
void aktualizuj_crc_stosu()
{
  pthread_mutex_lock(&watek);
  init_stos();
  int size=sizeof(stos),i=0,suma=0;
  stos.suma_kontrolna=0;
  while(1)
  {
    if(i==size) break;
    int value=*((char *)stos.start_brk+i);
    suma=suma+value;
    i++;
  }
  stos.suma_kontrolna=suma;
  pthread_mutex_unlock(&watek);
}

//funkcja zerujaca dane stosu
void reset_stosu()
{
  pthread_mutex_lock(&watek);
  intptr_t wielkosc_stosu=stos.brk-stos.start_brk;
  custom_sbrk(wielkosc_stosu*(-1));
  stos.start_brk=0;
  stos.brk=0;
  stos.stos_uzywane_miejsce=0;
  stos.stos_najwiekszy_uzywany=0;
  stos.stos_ilosc_uzywanych_blokow=0;
  stos.stos_wolne_miejsce=0;
  stos.stos_najwiekszy_wolny=0;
  stos.stos_wolne_bloki=0;
  pthread_mutex_unlock(&watek);

  pthread_mutexattr_destroy(&atrybut);
  pthread_mutex_destroy(&watek);
}

//funkcja wypelaniajaca tablice plotkow losowymi wartosciami
void wypeln_plotki_wartosciami()
{
  int i=0;
  int size=SIZE_OF_FENCE;
  size=size/sizeof(plotek.otwierajacy[0]);
  while(1)
  {
    if(i==size) break;
    plotek.otwierajacy[i]=rand();
    plotek.zamykajacy[i]=rand();
    i++;
  }
}

//inicjalizacja stosu
int heap_setup(void)
{
    pthread_mutexattr_init(&atrybut);
    pthread_mutexattr_settype(&atrybut, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&watek, &atrybut);
    pthread_mutex_lock(&watek);
    //robienie pamieci na start_brk i brk
    stos.start_brk = (intptr_t) custom_sbrk(SIZE_OF_MEMORY_BLOCK * 2);
    void *zm=(void*)stos.start_brk;
    if(zm==NULL)
    {
      pthread_mutex_unlock(&watek);
      return -1;
    } 
    stos.brk=stos.start_brk+SIZE_OF_MEMORY_BLOCK;

    init_start_brk();
    aktualizuj_crc((struct mblock *)stos.start_brk);

    init_brk();
    aktualizuj_crc((struct mblock *)stos.brk);

    stos.brk = stos.brk + SIZE_OF_MEMORY_BLOCK;
    
    wypeln_plotki_wartosciami();
    init_stos();
    aktualizuj_crc_stosu();

    pthread_mutex_unlock(&watek);
    return heap_validate();
}

//STANDARDOWE FUNKCJE
//funkcje odwoluja sie do funkcji DEBUG poniewaz jest to samo tylko fileline oraz filename sa ustawione na 0 i NULL

void* heap_malloc(size_t count)
{
  return heap_malloc_debug(count, 0, NULL);
}

void* heap_calloc(size_t number, size_t size)
{
  return heap_calloc_debug(number, size, 0, NULL);
}

void* heap_realloc(void* memblock, size_t size)
{
  return heap_realloc_debug(memblock, size, 0, NULL);
}

void heap_free(void* memblock)
{
  pthread_mutex_lock(&watek);
  //walidacja
  if(heap_validate()!=0) return;
  if(get_pointer_type(memblock)!=pointer_valid) return;
  //przesuniecie wskaznika
  intptr_t temp=(intptr_t)memblock-SIZE_OF_FENCE-SIZE_OF_MEMORY_BLOCK;
  struct mblock *p=(struct mblock*)temp;
  p->locked=0;//zerujemy 

  //jesli poprzedni blok jest wolny//przepisanie wskaznikow
  if(p->prev_block->locked!=1)
  {
    p->prev_block->next_block=p->next_block;
    p->next_block->prev_block=p->prev_block;
    p=p->prev_block;
  }

  //jesli nastepny blok jest wolny//przepisanie wskaznikow
  if(p->next_block->locked!=1)
  {
    p->next_block=p->next_block->next_block;
    p->next_block->prev_block=p;
  }
  intptr_t next=(intptr_t)p->next_block-SIZE_OF_MEMORY_BLOCK;
  intptr_t current=(intptr_t)p;
  int rozmiar=(int)(next-current);
  p->size=rozmiar;
  aktualizuj_wszystkie_crc(p);
  pthread_mutex_unlock(&watek);
}

//FUNKCJE DEBUG

//funkcja szukajaca bloku ktory bedzie spelnial oczekiwania odnosnie rozmiaru (czy juz istnieje)
struct mblock* znajdz(size_t count, struct mblock *p)
{
  while(1)
  {
    if(p==NULL) break;
    //przypadek w ktorym blok jest WOLNY i spelnia warunek na wielkosc miejsca
    if(p->locked==0)
    {
      size_t rozmiar=count+SIZE_OF_FENCE+SIZE_OF_FENCE+SIZE_OF_MEMORY_BLOCK;
      if(rozmiar<=p->size) return p;
    }
    //drugi przypadek w ktorym szukamy wolnego bloku
    if(p->locked==0)
    {
      size_t rozmiar=count+SIZE_OF_FENCE+SIZE_OF_FENCE;
      if(rozmiar==p->size) return p;
    }
    p=p->next_block;
  }
  return NULL;
}

//inicjalizuje stos w przypadku kiedy NULL (nie znajdziemy bloku ktory nas interesuje)
void init_stos_malloc_debug_null(struct mblock* p,int fileline,char* filename)
{
  struct mblock* temp=(struct mblock*)stos.brk;
  temp->size = 0;
  temp->locked = 1;
  temp->next_block = NULL;
  temp->prev_block = p;
  temp->fileline = fileline;
  temp->filename = (char *)filename;
  aktualizuj_crc(temp->prev_block);
  aktualizuj_crc(temp);
}

//inicjalizacja wskaznika o nowe dane
struct mblock* init_wskaznik_malloc_debug_null(struct mblock* p,int fileline,char* filename,size_t count)
{
  p->size=count;
  p->locked=1;
  p->next_block=(struct mblock *)stos.brk;
  p->fileline=fileline;
  p->filename=filename;
  aktualizuj_wszystkie_crc(p);
  p=ustaw_plotki(p,count);
  return p;
}

//funkcja obslugujaca przypadek kiedy nie znajdziemy bloku
void* malloc_debug_when_null(size_t count, int fileline, const char* filename)
{
  pthread_mutex_lock(&watek);
  struct mblock* p = (struct mblock *)(stos.brk-SIZE_OF_MEMORY_BLOCK);
  stos.brk = (intptr_t) custom_sbrk(SIZE_OF_MEMORY_BLOCK + SIZE_OF_FENCE * 2 + count);
  if ((void *)stos.brk==NULL) return NULL;
  stos.brk = stos.brk + count + SIZE_OF_FENCE + SIZE_OF_FENCE;
  init_stos_malloc_debug_null(p,fileline,(char*)filename);
  
  p=init_wskaznik_malloc_debug_null(p,fileline,(char*)filename,count);
  stos.brk = stos.brk + SIZE_OF_MEMORY_BLOCK;
  if(heap_validate()==0)
  {
    pthread_mutex_unlock(&watek);
    return (void *)((intptr_t)p + SIZE_OF_MEMORY_BLOCK + SIZE_OF_FENCE);
  } 
  pthread_mutex_unlock(&watek);
  return NULL; 
}

struct mblock* malloc_debug_when_not_null(struct mblock* p,size_t count,int fileline,char* filename)
{
  intptr_t new_p=(intptr_t)p;
  new_p=new_p+count+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE+SIZE_OF_FENCE;
  struct mblock* temp=(struct mblock*)new_p;
  temp->next_block=p->next_block;
  p->next_block->prev_block=temp;
  temp->prev_block=p;
  p->next_block=temp;
  temp->locked=0;
  int count_new=(int)count;
  count_new=count_new+SIZE_OF_FENCE+SIZE_OF_FENCE+SIZE_OF_MEMORY_BLOCK;
  temp->size=p->size-count_new;
  temp->fileline=fileline;
  temp->filename=filename;
  aktualizuj_wszystkie_crc(temp);
  return p;
}

//funkcja malloc debug
void* heap_malloc_debug(size_t count, int fileline, const char* filename)
{
  pthread_mutex_lock(&watek);
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  if(count==0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  struct mblock *p = (struct mblock *)stos.start_brk;
  //szukamy bloku ktory spelni oczekiwania odnosnie wielkosci
  p=znajdz(count,p);
  //jesli nie bedzie takiego bloku to musimy uzyc custom_brk i dodac pamieci
  if (p==NULL) 
  {
    pthread_mutex_unlock(&watek);
    return malloc_debug_when_null(count,fileline,filename);
  }
  //dodajemy blok jesli mamy jeszcze wystaraczajaco duzo miejsca
  if (p->size >= (int)(count + SIZE_OF_MEMORY_BLOCK + SIZE_OF_FENCE*2))
  {
    p=malloc_debug_when_not_null(p,count,fileline,(char*)filename);
  }
  intptr_t p_next=(intptr_t)p->next_block;
  intptr_t p_current=(intptr_t)p;
  p_current=p_current+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE+SIZE_OF_FENCE;
  p->size=(int)(p_next-p_current);
  p=ustaw_plotki(p,p->size);
  p->locked=1;
  p->fileline=fileline;
  p->filename=(char *)filename;
  aktualizuj_wszystkie_crc(p);
  if(heap_validate()==0)
  {
    pthread_mutex_unlock(&watek);
    return (void *)((intptr_t)p + SIZE_OF_MEMORY_BLOCK + SIZE_OF_FENCE);
  } 
  pthread_mutex_unlock(&watek);
  return NULL;
}

//zwykla funkcja calloc
void* heap_calloc_debug(size_t number, size_t size, int fileline, const char* filename)
{
  pthread_mutex_lock(&watek);
  //walidacja
  if(number==0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  if(size==0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  size_t total_size=number*size;
  //zwykly malloc
  void *p=heap_malloc_debug(total_size,fileline,filename);
  if(p==NULL)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  //zerujemy pamiec (charakterystyczna cecha calloca)
  wypeln_zerami(p,total_size);
  if(heap_validate()==0)
  {
    pthread_mutex_unlock(&watek);
    return p;
  }
  pthread_mutex_unlock(&watek);
  return NULL;
}

//zwykly realloc
void* heap_realloc_debug(void* memblock, size_t size, int fileline, const char* filename)
{
  pthread_mutex_lock(&watek);
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  //realloc(NULL,rozmiar)->alokacja nowego bloku
  if(memblock==NULL)
  {
    pthread_mutex_unlock(&watek);
    return heap_malloc_debug(size, fileline, filename);
  } 
  //realloc(blok,0)->zwalniamy pamiec
  if(size==0)
  {
    heap_free(memblock);
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  //przestawienie wskaznika
  intptr_t temp=(intptr_t)memblock;
  temp=temp-SIZE_OF_MEMORY_BLOCK-SIZE_OF_FENCE;
  //porownanie czy bedziemy zmniejszac blok czy powiekszac, przestawilismy juz wskaznik tak by dobrac sie do danych
  struct mblock *p = (struct mblock *)temp;
  int kopiowanie=0;
  if(p->size>size) kopiowanie=size;
  else kopiowanie=p->size;
  //zwalniamy stary blok
  heap_free(memblock);
  //alokowanie nowego bloku o tym rozmiarze
  void *alloc = heap_malloc_debug(size, fileline, filename);
  //jesli udalo sie zaalokowac to kopiujemy stare dane do nowego bloku
  if(alloc != NULL)
  {
    int i=0;
    char *t1=(char*)alloc;
    char *t2=(char*)memblock;
    while(i<kopiowanie)
    {
      *(t1+i)=*(t2+i);
      i++;
    }
    if(heap_validate()==0)
    {
      pthread_mutex_unlock(&watek);
      return alloc;
    }
  }
  pthread_mutex_unlock(&watek);
  return NULL;
}

//funkcja inicjalizujaca stos o wartosci
void init_stos_mal_aligned_null(int fileline,char* filename,struct mblock* p)
{
  struct mblock* temp=(struct mblock*)stos.brk;
  temp->size=0;
  temp->locked=1;
  temp->next_block=NULL;
  temp->prev_block=p;
  temp->filename=filename;
  temp->fileline=fileline;
  aktualizuj_crc(temp->prev_block);
  aktualizuj_crc(temp); 
}

//funkcja inicjalizujaca wskaznik "p" o nowe wartosci
struct mblock* init_wskaznik_mal_aligned_null(size_t count,int fileline,char* filename,struct mblock* prev_block,struct mblock* p)
{
  p->size=count;
  p->locked=1;
  p->next_block=(struct mblock *)stos.brk;
  p->prev_block=prev_block;
  prev_block->next_block=p;
  p->filename=filename;
  p->fileline=fileline;
  return p;
}
void* malloc_aligned_when_null(size_t count,int fileline,const char* filename)
{
  pthread_mutex_lock(&watek);
  struct mblock* p = (struct mblock *)(stos.brk - SIZE_OF_MEMORY_BLOCK);
  struct mblock *temp = p->prev_block;
  int current_size_of_heap=stos.brk-stos.start_brk;
  int modulo=current_size_of_heap % PAGE_SIZE;
  int space=PAGE_SIZE-modulo-SIZE_OF_FENCE;
  size_t needed_space=0;
  //w skrocie jesli zostaje nam kawa³ek strony z pamiecia musimy miec pewnosc ze zmieszcza sie tam dane w przeciwnym wypadku potrzebujemy kolejnej strony
  if(space>=0) needed_space=PAGE_SIZE-modulo;
  else needed_space=PAGE_SIZE-modulo+PAGE_SIZE;
  stos.brk = (intptr_t) custom_sbrk(SIZE_OF_MEMORY_BLOCK + SIZE_OF_FENCE + count + needed_space);
  if ((void *)stos.brk == NULL)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  } 
  //przestawienie wskaznika
  p = (struct mblock *)((intptr_t)p + needed_space - SIZE_OF_FENCE);
  //zwiekszenie stosu
  stos.brk = stos.brk + count + SIZE_OF_FENCE + needed_space;
  //inizjalizacja stosu (update)
  init_stos_mal_aligned_null(fileline, (char*)filename, p);
  //inizjalizacja wskaznika p (update)
  init_wskaznik_mal_aligned_null(count,fileline,(char *)filename,temp,p);
  //update wszystkich sum kontrolnych
  aktualizuj_wszystkie_crc(p);
  //funkcja ustawiajaca plotki
  p=ustaw_plotki(p,count);
  //zwiekszenie brk
  stos.brk=stos.brk+SIZE_OF_MEMORY_BLOCK;
  intptr_t size=(intptr_t)p;
  size=size+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE;
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  pthread_mutex_unlock(&watek);
  return (void *)size;
}
struct mblock* init_wskaznik_aligned_when_not_null(struct mblock* pointer,int fileline,char* filename,size_t count,struct mblock* p)
{
  pointer->next_block=p->next_block;
  p->next_block->prev_block=pointer;
  pointer->prev_block=p;
  p->next_block=pointer;
  pointer->locked=0;
  pointer->filename=filename;
  pointer->fileline=fileline;
  int size=(int)count;
  size=size+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE+SIZE_OF_FENCE;
  pointer->size=p->size-size;
  aktualizuj_wszystkie_crc(pointer);
  return p;
}

struct mblock* znajdz2(size_t count, struct mblock *p)
{
  while(1)
  {
    if(p==NULL) break;
    if(p->locked==0)
    {
      intptr_t temp=(intptr_t)p;
      if(p->size==(count+SIZE_OF_FENCE+SIZE_OF_FENCE))
      {
        int i=temp+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE;
        i=i%PAGE_SIZE;
        if(i==0) return p;       
      }
      if(p->size>=(count+SIZE_OF_FENCE+SIZE_OF_FENCE+SIZE_OF_MEMORY_BLOCK))
      {
        int i=temp+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE;
        i=i%PAGE_SIZE;
        if(i==0) return p;
      }
    }
    p = p->next_block;
  }
  return NULL;
}
void* heap_malloc_aligned_debug(size_t count, int fileline, const char* filename)
{
  pthread_mutex_lock(&watek);
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  if(count==0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  struct mblock *p = (struct mblock *)stos.start_brk;
  p=znajdz2(count,p);
  if(p==NULL)
  {
    pthread_mutex_unlock(&watek);
    return malloc_aligned_when_null(count,fileline,filename);
  } 

  int size=(int)count;
  size=size+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE+SIZE_OF_FENCE;
  //dodanie nowego bloku w przypadku posiadania miejsca
  if(p->size>=size)
  {
    intptr_t temp=(intptr_t)p;
    temp=temp+count+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE+SIZE_OF_FENCE;
    struct mblock* pointer=(struct mblock*)temp;
    p=init_wskaznik_aligned_when_not_null(pointer,fileline,(char*)filename,count,p);
  }
  intptr_t temp1=(intptr_t)p;
  temp1=temp1+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE+SIZE_OF_FENCE;
  intptr_t temp2=(intptr_t)p->next_block;
  size=(int)(temp2-temp1);
  p->size=size;
  p=ustaw_plotki(p,p->size);
  p->locked=1;
  p->filename=(char *)filename;
  p->fileline=fileline;
  aktualizuj_wszystkie_crc(p);
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  intptr_t r=(intptr_t)p;
  r=r+SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE;
  pthread_mutex_unlock(&watek);
  return (void *)r;
}

//funkcja calloc
void* heap_calloc_aligned_debug(size_t number, size_t size, int fileline, const char* filename)
{
  pthread_mutex_lock(&watek);
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  size_t total_size=number*size;
  void *memory=heap_malloc_aligned_debug(total_size,fileline,filename);
  if(memory==NULL)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  wypeln_zerami(memory,(int)total_size);
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  pthread_mutex_unlock(&watek);
  return memory;
}

void* heap_realloc_aligned_debug(void* memblock, size_t size, int fileline, const char* filename)
{
  pthread_mutex_lock(&watek);
  if(heap_validate()!=0)
  {
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  //zerowanie pamieci (kiedy size==0)
  if(size==0)
  {
    heap_free(memblock);
    pthread_mutex_unlock(&watek);
    return NULL;
  }
  //realloc w ktorym chcemy zaalokowac nowy blok
  if(memblock==NULL) return heap_malloc_aligned_debug(size, fileline, filename);
  if(get_pointer_type(memblock)==pointer_valid)
  {
    intptr_t temp=(intptr_t)memblock;
    temp=temp-SIZE_OF_MEMORY_BLOCK-SIZE_OF_FENCE;
    struct mblock *p=(struct mblock*)temp;
    int rozmiar=0;
    if(p->size>size) rozmiar=size;
    else rozmiar=p->size;
    //zwolnijmy stara pamiec
    heap_free(memblock);
    //zaalokujmy nowa (nowy rozmiar)
    void *alloc=heap_malloc_aligned_debug(size,fileline,filename);
    //przekopiujmy stare dane do nowego bloku
    if(alloc!=NULL)
    {
      int i=0;
      char *t1=(char*)alloc;
      char *t2=(char*)memblock;
      while(i<rozmiar)
      {
        *(t1+i)=*(t2+i);
        i++;
      }
    }
    if(heap_validate()==0)
    {
      pthread_mutex_unlock(&watek);
      return alloc;
    }
  }
  pthread_mutex_unlock(&watek);
  return NULL;
}

//STATYSTYKI

//funkcja zwraca liczbe wykorzystanych bajtow stosu
size_t heap_get_used_space(void)
{
    pthread_mutex_lock(&watek);
    struct mblock *p = (struct mblock *)stos.start_brk;
    int size = 0;
    while(1)
    {
      if(p==NULL) break;
      if(p->locked==1)
      {
        if(p->size==0)
        {
          size=size+SIZE_OF_MEMORY_BLOCK;
          p=p->next_block;
          continue;
        }
        size=size+p->size+SIZE_OF_FENCE+SIZE_OF_FENCE;
      }
      size=size+SIZE_OF_MEMORY_BLOCK;
      p=p->next_block;
    }
    pthread_mutex_unlock(&watek);
    return (size_t)size;
}

//funkcja zwraca najwiekszych uzywany blok (rozmiar)
size_t heap_get_largest_used_block_size(void)
{   
    pthread_mutex_lock(&watek);
    int max=0,size=0;
    struct mblock *p = (struct mblock *)stos.start_brk;
    while(1)
    {
      if(p==NULL) break;
      if(max<p->size)
      {
        max=p->size;
        if(p->locked==0) size=0;
        else size=p->size;
      }
      p=p->next_block;
    }
    pthread_mutex_unlock(&watek);
    return (size_t)size;
}

//funkcja zwraca ilosci zaalokowanych blokow
uint64_t heap_get_used_blocks_count(void)
{
    pthread_mutex_lock(&watek);
    struct mblock *p = (struct mblock *)stos.start_brk;
    if(p==NULL)
    {
      pthread_mutex_unlock(&watek);
      return 0;
    }
    uint64_t count=0;
    while(1)
    {
      if(p==NULL) break;
      if(p->size!=0)
      {
        if(p->locked!=0) count+=1; 
      }
      p=p->next_block;
    }
    pthread_mutex_unlock(&watek);
    return count;
}

//funkcja zwraca rozmiar wolnego miejsca
size_t heap_get_free_space(void)
{
    pthread_mutex_lock(&watek);
    struct mblock *p = (struct mblock *)stos.start_brk;
    if(p==NULL)
    {
      pthread_mutex_unlock(&watek);
      return 0;
    }
    int size=0;
    while(1)
    {
      if(p==NULL) break;
      if(p->locked!=1) size=size+p->size;
      p=p->next_block;
    }
    pthread_mutex_unlock(&watek);
    return (size_t)size;
}

//funkcja zwraca najwiekszy wolny obszar
size_t heap_get_largest_free_area(void)
{
    pthread_mutex_lock(&watek);
    struct mblock *p = (struct mblock *)stos.start_brk;
    if(p==NULL) 
    {
      pthread_mutex_unlock(&watek);
      return 0;
    }
    int max=0,size=0;
    while(1)
    {
      if(p==NULL) break;
      if(p->locked==1)
      {
        p=p->next_block;
        continue;
      }
      if(max<p->size) max=p->size;
      p=p->next_block;
    }
    pthread_mutex_unlock(&watek);
    return (size_t)size;
}

//funkcja zwraca ilosc dziur
uint64_t heap_get_free_gaps_count(void)
{
    pthread_mutex_lock(&watek);
    struct mblock *p=(struct mblock *)stos.start_brk;
    uint64_t gaps=0;
    while(1)
    {
      if(p==NULL) break;
      if(p->locked==1)
      {
        p=p->next_block;
        continue;
      }
      if(SIZE_OF_FENCE+SIZE_OF_FENCE+WORD_SIZE<=p->size) gaps=gaps+1;
      p=p->next_block;
    }
    pthread_mutex_unlock(&watek);
    return gaps;
}

//RESZTA FUNKCJI

enum pointer_type_t get_pointer_type(const void* pointer)
{
    pthread_mutex_lock(&watek);
    if (pointer == NULL)
    {
      pthread_mutex_unlock(&watek);
      return pointer_null;
    } 

    intptr_t roznica=(intptr_t)pointer-stos.start_brk;
    //pointer nie moze byc przed start_brk
    if(roznica<0)
    {
      pthread_mutex_unlock(&watek);
      return pointer_out_of_heap;
    }

    //nizej jest wyjasnione czemu to nie jest konieczne
/*
    roznica=stos.brk-(intptr_t)pointer;
    //pointer nie moze byc za brk
    if(roznica<0) return pointer_out_of_heap;*/

    struct mblock * p = (struct mblock *)stos.start_brk;
    intptr_t pointer_copy = (intptr_t)pointer;
    intptr_t this_block = (intptr_t)pointer;

    //iterujemy sie (przechodzimy po) blokach do przodu
    while(1)
    {
      if(p==NULL)
      {
        if(this_block<=(stos.brk-SIZE_OF_MEMORY_BLOCK))
        {
          pthread_mutex_unlock(&watek);
          return pointer_control_block; 
        }
        break;
      }

      //szukamy bloku ktory jest "TYM" roznica bedzie ujemna to znaczy ze jestesmy juz za daleko
      intptr_t temp=(intptr_t)p;
      roznica=this_block-temp;
      if(roznica<0)
      {
        //cofamy sie do poprzedniego.. czyli "TEGO KONKRETNEGO"
        p=p->prev_block;
        temp=(intptr_t)p;
        //CZY JEST WOLNY?
        if(p->locked==0)
        {
          if(temp==this_block)
          {
            pthread_mutex_unlock(&watek);
            return pointer_control_block;//wskazuje na obszar struktur wewenetrznych
          }
          if(this_block==temp+SIZE_OF_MEMORY_BLOCK)
          {
            pthread_mutex_unlock(&watek);
            return pointer_valid;//idealny "W PUNKT"//jest locked==0 wiec nie dodajemy plotka
          }

          //odejmujemy to co daje nam "VALID" i jesli jest mniejsze od 0 to znaczy ze wskazuje na struktury wew.
          roznica=this_block-temp-SIZE_OF_MEMORY_BLOCK;
          if(roznica<0)
          {
            pthread_mutex_unlock(&watek);
            return pointer_control_block;
          }

          //jesli tutaj odejmiemy to co wczesniej i dodatkowe rozmiar to w przypadku roznicy <0 mamy wolny blok
          roznica=this_block-temp-SIZE_OF_MEMORY_BLOCK-p->size;
          if(roznica<0)
          {
            pthread_mutex_unlock(&watek);
            return pointer_unallocated;
          }
        }
        //JEST ZAJETY??
        if(p->locked==1)
        {
          if(temp==this_block)
          {
            pthread_mutex_unlock(&watek);
            return pointer_control_block;//wskazuje na obszar struktur wew.
          }
          int size=SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE;//dodajemy plotek z memory bo jest locked==1
          if(this_block==temp+size)
          {
            pthread_mutex_unlock(&watek);
            return pointer_valid;//"IDEALNY W PUNKT"
          }

          //odejmiemy to co daje nam idealny "W PUNKT" jak mniejszy od 0 to znaczy ze struktury wew.
          roznica=this_block-temp-size;

          if(roznica<0)
          {
            pthread_mutex_unlock(&watek);
            return pointer_control_block;
          }

          roznica-=p->size;//odejmiemy jeszcze rozmiar jak <0 to znaczy ze wskazuje na dane (w srodek czegos)
          if(roznica<0)
          {
            pthread_mutex_unlock(&watek);
            return pointer_inside_data_block;
          }
        }

      }
      p=p->next_block;
    }
    //przeszlismy od start_brk do konca iiii nie bylo go.. wiec jest za brk
    pthread_mutex_unlock(&watek);
    return pointer_out_of_heap;
}

////////////////////////////////////////////////////////
//Funkcja zwraca wskaŸnik na pocz¹tek bloku, na którego dowolny bajt wskazuje pointer

void* heap_get_data_block_start(const void* pointer)
{   
    pthread_mutex_lock(&watek);
    if(get_pointer_type(pointer)!=pointer_inside_data_block)
    {
      pthread_mutex_unlock(&watek);
      return NULL;
    }
    struct mblock* p=(struct mblock*)stos.start_brk; 
    if(get_pointer_type(pointer)==pointer_valid) return (void*)pointer; 
    intptr_t block_pointer = (intptr_t)pointer;
    while(1)
    {
      if(p==NULL) break;
      intptr_t temp=(intptr_t)p;
      intptr_t roznica=block_pointer-temp;
      if(roznica<0)
      {
        int size=SIZE_OF_MEMORY_BLOCK+SIZE_OF_FENCE;
        intptr_t r= (intptr_t)p->prev_block+size;
        return (void*)r;
      }
    }
    pthread_mutex_unlock(&watek);
    return NULL;
}

///////////////////////////////////////////////

//Funkcja zwraca d³ugoœæ bloku danego wskaŸnikiem memblock.  Je¿eli memblocknie wskazuje na zaalo-kowany blok (inny ni¿ pointer_valid), to funkcja zwraca 0.
size_t heap_get_block_size(const void* memblock)
{
    pthread_mutex_lock(&watek);
    if ((get_pointer_type(memblock) != pointer_valid))
    {
      pthread_mutex_unlock(&watek);
      return 0;
    }
    intptr_t block_pointer=(intptr_t)memblock;
    struct mblock * p = (struct mblock *)stos.start_brk;
    while(1)
    {
      if(p==NULL) break;
      intptr_t roznica=block_pointer-(intptr_t)p;//w momencie w ktorym roznica bedzie ujemna bedziemy wiedziec ze jestesmy juz za blokiem ktory nas interesuje
      if(roznica<0)
      {
        size_t r=p->prev_block->size;
        return r;
      }
      p=p->next_block;
    }
    pthread_mutex_unlock(&watek);
    return 0;
}

//////////////////////////////////////////

//wyswietla statystyki stosu
void wyswietl_dane_stosu()
{
  printf("\n");
  printf("Dane na temat stosu!!!");
  printf("\n");
  printf("-------------------------------\n");
  size_t rozmiar=stos.brk-stos.start_brk;
  size_t uzywane=heap_get_used_space();
  size_t wolne=heap_get_free_space();
  size_t wolne_max=heap_get_largest_free_area();
  printf("Rozmiar stosu:%zu  Uzywane:%zu  ",rozmiar,uzywane);
  printf("Wolne:%zu  Wolny(MAX):%zu\n",wolne,wolne_max);
}

//wyswietla bloki (kolejne) w raz z nazwa pliku i numerem linii
void wyswietl_bloki()
{
  printf("\n");
  printf("Zaczynam wyswietlac bloki pamieci!!");
  printf("\n");
  printf("-------------------------------\n");
  struct mblock *p=(struct mblock*) stos.start_brk;
  int i=1;
  while(1)
  {
    if(p==NULL) break;
    if(p->locked==1)
    {
      printf("%d.  Adres: %ld  Rozmiar: %d  ",i,(long int)p, p->size);
      if(p->filename==NULL) printf("Plik: [BRAK]  Linia: [BRAK]\n");
      else printf("Plik: %s  Linia: %d\n",p->filename,p->fileline);
      i++;
    }
    
    p=p->next_block;
  }
}

//robi skumulowane funkcje (obie powyzsze)
void heap_dump_debug_information(void)
{
  pthread_mutex_lock(&watek);
  if(heap_validate())
  {
    pthread_mutex_unlock(&watek);
    printf("\nStos jest uszkodzony!!!!\n");
    return;
  }
  wyswietl_dane_stosu();
  wyswietl_bloki();
  pthread_mutex_unlock(&watek);
}

/////////////////////////////////////////////

//porownuje dane statystyczne zapisane na stosie z tymi, ktore zostana otrzymane
//w wyniku dzialania funkcji (czyli faktyczna iloscia danych)
int porownaj_bloki()
{
  if(stos.stos_uzywane_miejsce!=heap_get_used_space()) return 1;
  if(stos.stos_najwiekszy_uzywany!=heap_get_largest_used_block_size()) return 1;
  if(stos.stos_ilosc_uzywanych_blokow!=heap_get_used_blocks_count()) return 1;
  if(stos.stos_wolne_miejsce!=heap_get_free_space()) return 1;
  if(stos.stos_najwiekszy_wolny!=heap_get_largest_free_area()) return 1;
  if(stos.stos_wolne_bloki!=heap_get_free_gaps_count()) return 1;  
  return 0;
}
/*
int sprawdz_crc_wszystkich_blokow()
{
  struct mblock *p=(struct mblock*) stos.start_brk;
  while(1)
  {
    if(p==NULL) break;
    int i=0,nowe=0,size=sizeof(struct mblock),stare=p->crc;
    //printf("stare: %d\n",stare);
    while(1)
    {
      if(i==size) break;
      int value=*((char*)p+i);
      nowe=nowe+value;
      i++;
    }  
    //printf("nowe: %d\n",nowe-stare);
    if(nowe-stare!=stare) return 1;
    p=p->next_block;
  }
  return 0;
}*/
//robimy kopie zapasowa danych (sumy kontrolnej starej) i przechodzimy kolejno
//sumujac dane i sprawdzajac czy sumy kontrolne sa sobie rowne
int porownaj_crc()
{
  //robienie m.in. kopii zapasowej CRC
  int stare=stos.suma_kontrolna,nowe=0,i=0,size=sizeof(stos);
  stos.suma_kontrolna=0;
  //przechodzenie po kolejnych elementach i sumowanie liczby
  while(1)
  {
    if(i==size) break;
    int value=*((char*)stos.start_brk+i);
    nowe=nowe+value;
    i++;
  }
  //sprawdzenie obu crc
  //stos.suma_kontrolna=nowe;
  //printf("stare: %d, nowe: %d\n",stare,nowe);
  if(stare!=nowe) 
  {
    stos.suma_kontrolna=nowe;
    return 1;
  } 
  stos.suma_kontrolna=nowe;
  //if(sprawdz_crc_wszystkich_blokow()) return 1;
  return 0;
}



//sprawdzenie czy nastepny/poprzepdni wskaznik nie wychodzi po za stos
int porownaj_wskazniki(struct mblock* p)
{
  if(p==NULL) return 1;
  if(get_pointer_type(p->next_block)==pointer_out_of_heap) return 1;
  if(get_pointer_type(p->prev_block)==pointer_out_of_heap) return 1;
  return 0;
}

//funkcja oparta o memcmp ale zwraca wartosci 0 lub 1 (wiecej nam nie potrzeba)
int porownywanie_danych(void* p1,void* p2,int len)
{
  unsigned char* t1=p1;
  unsigned char* t2=p2;
  int i=0;
  while(1)
  {
    if(i==len) break;
    if(*(t1+i)!=*(t2+i)) return 1;
    i++;
  }
  return 0;
}

//funkcja ktora ma za zadanie sprawdzic poprawnosc plotkow (obu)
//iteruje odpowiednio wskaznik oraz wykonuje porownywanie
int porownaj_plotki(struct mblock* p)
{
  if(p==NULL) return 1;
  intptr_t pointer=(intptr_t)p;
  pointer=pointer+SIZE_OF_MEMORY_BLOCK;
  int y=porownywanie_danych((void*)pointer,plotek.otwierajacy,SIZE_OF_FENCE);
  if(y) return 1;
  pointer=pointer+SIZE_OF_FENCE+p->size;
  y=porownywanie_danych((void*)pointer,plotek.zamykajacy,SIZE_OF_FENCE);
  if(y) return 1;
  return 0;
} 

int aktualizuj_crc2(struct mblock *p)
{
  if(p==NULL) return 1;
  pthread_mutex_lock(&watek);
  int size=SIZE_OF_MEMORY_BLOCK,suma=0,i=0;
  int stare=p->crc;
  p->crc=0;
  while(1)
  {
    if(i==size) break;
    int value=*((char*)p+i);
    suma=suma+value;
    i++;
  }
  p->crc=suma;
  if(suma!=stare)
  {
    pthread_mutex_unlock(&watek);
    return 1;
  }
  pthread_mutex_unlock(&watek);
  aktualizuj_crc_stosu();
  return 0;
}

int zmien_wszystkie_crc()
{
  struct mblock *p=(struct mblock*) stos.start_brk;
  int i=0;
  while(1)
  {
    if(p==NULL) break;
    if(aktualizuj_crc2(p)) return 1;
    p=p->next_block;
  }  
  return 0;
}

//walidacja stosu 
int heap_validate(void)
{
    pthread_mutex_lock(&watek);
    //nieustawiony stos
    if ((void *)stos.start_brk == NULL || (void *)stos.brk == NULL)
    {
      pthread_mutex_unlock(&watek);
      return -1;
    }
    //blad CRC
    if (porownaj_crc())
    {
      pthread_mutex_unlock(&watek);
      return -2;
    }
    //zle dane stosu
    if (porownaj_bloki())
    {
      pthread_mutex_unlock(&watek);
      return -3;
    }
    if(zmien_wszystkie_crc())
    {
      pthread_mutex_unlock(&watek);
      return -2;     
    }
    struct mblock *p = (struct mblock *) stos.start_brk;
    while(1)
    {
        if(p==NULL) break;
        //blad plotkow
        if (porownaj_plotki(p)&& p->locked && p->size != 0)
        {
          pthread_mutex_unlock(&watek);
          return -4;
        }
        //blad pointeraznikow nastepnego/poprzedniego
        if (porownaj_wskazniki(p))
        {
          pthread_mutex_unlock(&watek);
          return -5;
        }
        p = p->next_block;      
    }
    pthread_mutex_unlock(&watek);
    return 0;
}

////////////////////////////////////////





