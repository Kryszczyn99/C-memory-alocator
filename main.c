
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "alokator.h"

//STRONA to 4KB -> 4*1024 
#define PAGE_SIZE 4096

//define zgodny z projektem (tego jak bylo opisane by to zrobic)
#define heap_malloc(size) heap_malloc_debug(size, __LINE__, __FILE__)
#define heap_malloc_aligned(size) heap_malloc_aligned_debug(size, __LINE__, __FILE__)

#define heap_calloc(size, num) heap_calloc_debug(size, num, __LINE__, __FILE__)
#define heap_calloc_aligned(size, num) heap_calloc_aligned_debug(size, num, __LINE__, __FILE__)

#define heap_realloc(ptr, size) heap_realloc_debug(ptr, size, __LINE__, __FILE__)
#define heap_realloc_aligned(ptr, size) heap_realloc_aligned_debug(ptr, size, __LINE__, __FILE__)


int porownaj(char* tab1,char* tab2,int size)
{
  int i=0;
  while(1)
  {
    if(i==size) break;
    if(tab1[i]!=tab2[i]) return 1;
    i++;
  }
  return 0;
}
void kopiuj(char* dst,char* src,int size)
{
  int i=0;
  while(1)
  {
    if(i==size) break;
    dst[i]=src[i];
    i++;
  }
}

int roznica_brk()
{
  int i=stos.brk - stos.start_brk;
  if(i==0) return 0;
  return 1;
}

int restart()
{
    reset_stosu();
    if(roznica_brk())
    {
      printf("Roznica brk sie nie zgadza\n");
      return 1;
    }
    int status = heap_setup();
    if(status!=0)
    {
      printf("Wartosc sie nie zgadza!\n");
      return 1;
    }
    return 0;
}
//alokacja uzywajac malloca
int test1()
{
      printf("TEST1 => [START]\n");
      int status = heap_setup();
      if(status!=0)
      {
        printf("Wartosc sie nie zgadza!\n");
        return 1;
      }
      int size=100*sizeof(char*);
      char** arr = (char **)heap_malloc(size);
      if(arr==NULL)
      {
        printf("Zmienna posiada wartosc NULL\n");
        return 1;
      }
      intptr_t old_p[100];

      for (int i = 0; i<100; ++i)
      {
          arr[i] = (char *)heap_malloc(sizeof(char) * ((i+1)*100));
          if(arr[i]==NULL)
          {
            printf("Element tablicy jest NULL\n");
            return 1;
          }
      }

      printf("TEST1 => [POMYŒLNY]\n\n");
      return 0;
}

//alokacja uzywajac calloca
int test2()
{
      printf("TEST2 => [START]\n");
      int size=100*sizeof(char*);
      char** arr = (char **)heap_calloc(sizeof(char),size);
      if(arr==NULL)
      {
        printf("Zmienna posiada wartosc NULL\n");
        return 1;
      }
      intptr_t old_p[100];

      for (int i = 0; i<100; ++i)
      {
          arr[i] = (char *)heap_calloc(sizeof(char),((i+1)*100));
          if(arr[i]==NULL)
          {
            printf("Element tablicy jest NULL\n");
            return 1;
          }
      }

      printf("TEST2 => [POMYŒLNY]\n\n");
      return 0;
}

//zwalnianie pamieci po zaalokowaniu oraz nadpisywanie plotków
int test3()
{
      printf("TEST3 => [START]\n");
      int size=100*sizeof(char*);
      char** arr = (char **)heap_calloc(sizeof(char),size);
      if(arr==NULL)
      {
        printf("Zmienna posiada wartosc NULL\n");
        return 1;
      }
      intptr_t old_p[100];

      for (int i = 0; i<100; ++i)
      {
          arr[i] = (char *)heap_calloc(sizeof(char),((i+1)*100));
          if(arr[i]==NULL)
          {
            printf("Element tablicy jest NULL\n");
            return 1;
          }
      }
      for (int i = 0; i<100; ++i)
      {
        old_p[i] = (intptr_t )arr[i];
        heap_free(arr[i]);
        if(get_pointer_type(arr[i]) == pointer_valid)
        {
          printf("B³ad wskaznika!\n");
          return 1;
        }
        
      }
      for (int i = 0; i<100; ++i)
      {
          arr[i] = (char *)heap_malloc(sizeof(char) * ((i+1)*100));
          if(arr[i]==NULL)
          {
            printf("Element tablicy jest NULL\n");
            return 1;
          }
          intptr_t temp=(intptr_t)arr[i];
          if(temp!=old_p[i])
          {
            printf("Elementy sie nie zgadzaja\n");
            return 1;
          }
      }
      strcpy(arr[0], "");
      for (int i = 0; i<100; ++i)
      {
          strcat(arr[0], "a");
      }
      int value = heap_validate();
      if(value==0)
      {
        printf("Test nie przeszedl pomyslnie\n");
        return 1;
      }
      printf("TEST3 => [POMYŒLNY]\n\n"); 
      return 0;
}

//free test 2, test kolizji
int test4()
{
    printf("TEST4 => [START]\n");
    int gaps=heap_get_free_gaps_count();
    if(gaps!=0)
    {
      printf("Ilosc wolnych miejsc sie nie zgadza\n");
      return 1;
    }
    char* arr2[100];
    

    for (int i = 0; i<100; ++i)
    {
        arr2[i] = (char *)heap_malloc(sizeof(char) * 1000);
        if(get_pointer_type(arr2[i])!=pointer_valid)
        {
          printf("Wskaznik nie jest valid\n");
          return 1;
        }
    }
    for (int i = 1; i<100; i += 10)
    {
        heap_free(arr2[i]);
        if(get_pointer_type(arr2[i])==pointer_valid)
        {
          printf("Wskaznik jest valid\n");
          return 1;
        }
    }
    gaps=heap_get_free_gaps_count();
    if(gaps!=10)
    {
      printf("Ilosc wolnych miejsc sie nie zgadza x2\n");
      return 1;
    }
    printf("TEST4 => [POMYŒLNY]\n\n");
    return 0;
}

//testy reallocu 
int test5()
{
    printf("TEST5 => [START]\n");
    int *a = (int *)heap_realloc(NULL, sizeof(int));
    if(a==NULL)
    {
      printf("Wartosc jest rowna NULL\n");
      return 1;
    }
    *a = 1;
    int value = *a;
    if(value!=1)
    {
      printf("Wartosc jest zla\n");
      return 1;
    }
    
    //ustawienie stosu na 0
    heap_realloc(a, 0);
    if(get_pointer_type(a)==pointer_valid)
    {
      printf("Wskaznik sie nie zgadza\n");
      return 1;
    }
    if(heap_validate()!=0)
    {
      printf("Walidacja stosu jest bledna\n");
      return 1;
    }
    if(heap_get_used_blocks_count()!=0)
    {
      printf("Uzywane bloki sie nie zgadzaja\n");
      return 1;
    }

    char tab[]="Ala ma kota";
    int size=sizeof(tab);
    char *tab_realloc=(char*)heap_realloc(NULL,size);
    if(tab_realloc==NULL)
    {
      printf("Alokacja nie powiodla sie\n");
      return 1;
    }
    kopiuj(tab_realloc,tab,size);
    tab_realloc=(char*)heap_realloc(tab_realloc,size/2);
    if(tab_realloc==NULL)
    {
      printf("Alokacja nie powiodla sie x2\n");
      return 1;
    }
    if(porownaj(tab,tab_realloc,size/2))
    {
      printf("Elementy tabeli powinny sie zgadzac\n");
      return 1;
    }
    if(!porownaj(tab,tab_realloc,size))
    {
      printf("Niektore elementy tabeli powinny sie zgadzac");
      return 1;
    }  
    printf("TEST5 => [POMYŒLNY]\n\n");
    return 0;
}

//malloc_aligned
int test6()
{
  printf("TEST6 => [START]\n");
  char** b = (char **)heap_malloc_aligned(sizeof(char*)*10);
  if(b==NULL) return 1;
  for (int i=0;i<10;i++)
  {
      b[i] = (char *)heap_malloc_aligned(sizeof(char) *10);
      if(get_pointer_type(b[i])!=pointer_valid) return 1;
      //test z dokumentacji do projektu
      if(((intptr_t)b[i] & (intptr_t)(PAGE_SIZE - 1))!=0) return 1;
  }
  for (int i=0;i<10;i++)
  {
      heap_free(b[i]);
      if(get_pointer_type(b[i])==pointer_valid) return 1;
  }

  printf("TEST6 => [POMYŒLNY]\n\n");
  return 0;
}

int test7()
{
  printf("TEST7 => [START]\n");
  char** b = (char **)heap_calloc_aligned(sizeof(char*),10);
  if(b==NULL) return 1;
  for (int i=0;i<10;i++)
  {
      b[i] = (char *)heap_calloc_aligned(sizeof(char),10);
      if(get_pointer_type(b[i])!=pointer_valid) return 1;
      //test z dokumentacji do projektu
      if(((intptr_t)b[i] & (intptr_t)(PAGE_SIZE - 1))!=0) return 1;
  }
  for (int i=0;i<10;i++)
  {
      heap_free(b[i]);
      if(get_pointer_type(b[i])==pointer_valid) return 1;
  }

  printf("TEST7 => [POMYŒLNY]\n\n");
  return 0;
}

//realloc_aligned
int test8()
{
  printf("TEST8 => [START]\n");
  char *text=(char*)heap_realloc_aligned(NULL,sizeof(char));
  if(text==NULL) return 1;
  if(((intptr_t)text & (intptr_t)(PAGE_SIZE - 1))!=0) return 1;
  *text='X';
  if(*text!='X') return 1;
  heap_realloc_aligned(text,0);
  if(get_pointer_type(text)==pointer_valid) return 1;
  if(heap_get_used_blocks_count()!=0) return 1;
  if(heap_validate()) return 1;

    char tab[]="Ala ma kota";
    int size=sizeof(tab);
    char *tab_realloc=(char*)heap_realloc_aligned(NULL,size);
    if(tab_realloc==NULL)
    {
      printf("Alokacja nie powiodla sie\n");
      return 1;
    }
    if(((intptr_t)tab_realloc & (intptr_t)(PAGE_SIZE - 1))!=0) return 1;
    kopiuj(tab_realloc,tab,size);
    tab_realloc=(char*)heap_realloc_aligned(tab_realloc,size/2);
    if(((intptr_t)tab_realloc & (intptr_t)(PAGE_SIZE - 1))!=0) return 1;
    if(tab_realloc==NULL)
    {
      printf("Alokacja nie powiodla sie x2\n");
      return 1;
    }
    if(porownaj(tab,tab_realloc,size/2))
    {
      printf("Elementy tabeli powinny sie zgadzac\n");
      return 1;
    }
    if(!porownaj(tab,tab_realloc,size))
    {
      printf("Niektore elementy tabeli powinny sie zgadzac");
      return 1;
    }  

  printf("TEST8 => [POMYŒLNY]\n\n");
  return 0;
}
//40//32//50//32
//testy alokowania i zwalniamy//testowanie funkcji statystycznych
int test9()
{
  printf("TEST9 => [START]\n");
  char* ptr1=(char*)heap_malloc(100);
  char* ptr2=(char*)heap_malloc(200);
  heap_dump_debug_information();
  int size=heap_get_largest_used_block_size();
  if(size!=200)
  {
    printf("Najwiekszy blok jest inny!\n");
    return 1;
  }
  size=heap_get_used_blocks_count();
  if(size!=2)
  {
    printf("Ilosc blokow ktore sa uzywane sie nie zgadza!\n");
    return 1;
  }
  heap_free(ptr1);
  size=heap_get_used_blocks_count();
  if(size!=1)
  {
    printf("Ilosc blokow ktore sa uzywane sie nie zgadza!\n");
    return 1;
  }
  if(heap_validate()!=0)
  {
    printf("Blad!");
    return 1;
  }
  heap_dump_debug_information();
  ptr1=(char*)heap_malloc(50);
  heap_dump_debug_information();
  printf("TEST9 => [POMYŒLNY]\n\n");
  return 0;
}

//testowanie funkcji na watkach// debug
void* alokacja()
{
  char* ptr1=(char*)heap_malloc(50);
  char* ptr2=(char*)heap_calloc(25,2);
  char* ptr3=(char*)heap_realloc(NULL,10);
  heap_free(ptr3);
  heap_free(ptr1);
  heap_free(ptr2);
  return NULL;
}
int test10()
{
  printf("TEST10 => [START]\n");
  pthread_t tab[20];
  int i=0;
  while(1)
  {
    if(i==20) break;
    pthread_create(&tab[i],NULL,alokacja,NULL);
    i=i+1;
  }
  if(heap_validate()!=0) 
  {
    printf("Cos poszlo nie tak!\n");
    return 1;
  }
  printf("TEST10 => [POMYŒLNY]\n\n");
  return 0;
}

//testowanie funkcji na watkach// aligned 
void* alokacja2()
{
  char* ptr1=(char*)heap_malloc_aligned(4096);
  heap_free(ptr1);
  return NULL;
}
int test11()
{
  printf("TEST11 => [START]\n");
  pthread_t tab[20];
  int i=0;
  while(1)
  {
    if(i==20) break;
    pthread_create(&tab[i],NULL,alokacja2,NULL);
    i=i+1;
  }
  if(heap_validate()!=0) 
  {
    printf("Cos poszlo nie tak!\n");
    return 1;
  }
  printf("TEST11 => [POMYŒLNY]\n\n");
  return 0;
}

//MEMORY//FENCE//DATAS//FENCE
//heap_validate, testy plotkow
int test12()
{
  printf("TEST12 => [START]\n");
  //FBBBBBBBBBBF (przed i po jest plotek)
  char* ptr=heap_calloc(10,1);
  //przed ptr jest plotek iwec odejmy 1 by zmienic do niego dane
  unsigned char* size=(unsigned char*)ptr;
  char* ptr2=(size-1);
  //uszkadzamy plotek
  *ptr2=1; 
  if(heap_validate()!=-4)
  {
    printf("Plotek nie zostal uszkodzony a mial!\n");
    return 1;
  }
  heap_free(ptr);
  printf("TEST12 => [POMYŒLNY]\n\n");
  return 0;  
}

//heap_validate, naruszenie bloku danych
int test13()
{
  printf("TEST13 => [START]\n");
  //MEMFBBBBBBBBBBF (przed i po jest plotek)
  char* ptr=heap_calloc(10,1);
  //przed ptr jest plotek iwec odejmy 1 by zmienic do niego dane
  unsigned char* size=(unsigned char*)ptr;
  struct mblock* ptr2=(size-sizeof(plotek.otwierajacy)-sizeof(struct mblock));//liczba by trafic w pole pamieci
  //uszkadzamy CRC
  printf("%d\n",ptr2->crc);
  size[-33]=100;
  printf("%d\n",ptr2->crc);
  if(heap_validate()!=-2)
  {
    printf("CRC nie zostal uszkodzony a mial!\n");
    return 1;
  }
  heap_free(ptr);
  printf("TEST13 => [POMYŒLNY]\n\n");
  return 0;  
}

//wskazniki funkcja enum
int test14()
{
  printf("TEST14 => [START]\n");
  if(get_pointer_type(NULL)!=pointer_null)
  {
    printf("Pointer powinien byc null!\n");
    return 1;
  }
  char tekst[]="Ala ma kota";
  if(get_pointer_type(tekst)!=pointer_out_of_heap)
  {
    printf("Pointer powinien wskazywac po za stos\n");
    return 1;
  }
  char *ptr=(char*)heap_malloc(100);
  if(ptr==NULL || get_pointer_type(ptr)!=pointer_valid)
  {
    printf("Cos poszlo nie tak!\n");
    return 1;
  }
  
  //MEM//FENCE//DATAS//FENCE
  
  intptr_t size=(intptr_t)ptr;
  size=size+10;//wchodzimy w dane
  void* dane=(void*)size;
  if(get_pointer_type(dane)!=pointer_inside_data_block)
  {
    printf("Pointer powinien wskazywac na blok danych\n");
    return 1;
  }
  int rozmiar=sizeof(struct mblock)+sizeof(plotek.otwierajacy);
  intptr_t pointer=(intptr_t)ptr-rozmiar;
  if(get_pointer_type((void*)pointer)!=pointer_control_block)
  {
    printf("Wskaznik nie wskazuje na control blocka\n");
    return 1;
  }
  printf("TEST14 => [POMYŒLNY]\n\n");
  return 0;  
}


//used_space test
int test15()
{
  printf("TEST15 => [START]\n");
  //40==sizeof(struct mblock)
  //32==plotek jeden
  //na start mamy tylko 80 bo mamy dwa mblocki brk i startbrk 40+40 
  if(heap_get_used_space()!=80)
  {
    printf("Rozmiar sie nie zgadza\n");
    return 1;
  }
  heap_malloc(100);
  //teraz matematyka
  //mamy 80 ze starego i nowy blok to 40(mblock)32+32(plotki)100(pamiec) razem 284
  if(heap_get_used_space()!=284)
  {
    printf("Rozmiar po zaalokowaniu sie nie zgadza\n");
    return 1;
  }
  printf("TEST15 => [POMYŒLNY]\n\n");
  return 0;  
}


int main(int argc, char **argv)
{
    printf("\n TESTY JEDNOSTKOWE START\n\n");
/*
    if(test1()) return 0;
    if(restart()) return 0;

    if(test2()) return 0;
    if(restart()) return 0;

    if(test3()) return 0;
    if(restart()) return 0;

    if(test4()) return 0;
    if(restart()) return 0;

    if(test5()) return 0;
    if(restart()) return 0;

    if(test6()) return 0;
    if(restart()) return 0;

    if(test7()) return 0;
    if(restart()) return 0;
  
    if(test8()) return 0;
    if(restart()) return 0;

    if(test9()) return 0;
    if(restart()) return 0;

    if(test10()) return 0;
    if(restart()) return 0;

    if(test11()) return 0;
    if(restart()) return 0;

    if(test12()) return 0;
    if(restart()) return 0;
*/
    heap_setup();//w razie odkomentowania reszty to trzeba zakomentowac
    if(test13()) return 0;
    if(restart()) return 0;

    if(test14()) return 0;
    if(restart()) return 0;

    if(test15()) return 0;
    reset_stosu();
    printf("\n WSZYSTKIE TESTY PRZESZLY POMYSLNIE\n");
    return 0;
}

