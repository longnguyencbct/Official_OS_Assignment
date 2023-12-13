// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t memphy_mutex;

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
   pthread_mutex_lock(&memphy_mutex);
   int numstep = 0;

   mp->cursor = 0;
   while (numstep < offset && numstep < mp->maxsz)
   {
      /* Traverse sequentially */
      mp->cursor = (mp->cursor + 1) % mp->maxsz;
      numstep++;
   }
   pthread_mutex_unlock(&memphy_mutex);
   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   
   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */
   
   pthread_mutex_lock(&memphy_mutex);
   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE)mp->storage[addr];
   pthread_mutex_unlock(&memphy_mutex);
   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   if (mp->rdmflg) {
      pthread_mutex_lock(&memphy_mutex);
      *value = mp->storage[addr];
      pthread_mutex_unlock(&memphy_mutex);
   } else {
      return MEMPHY_seq_read(mp, addr, value);
   } /* Sequential access device */
      
   
   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct *mp, int addr, BYTE value)
{
   
   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   pthread_mutex_lock(&memphy_mutex);
   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;
   pthread_mutex_unlock(&memphy_mutex);
   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct *mp, int addr, BYTE data)
{
   
   if (mp == NULL)
      return -1;

   if (mp->rdmflg) {
      pthread_mutex_lock(&memphy_mutex);
      mp->storage[addr] = data;
      pthread_mutex_unlock(&memphy_mutex);
   } else {
      return MEMPHY_seq_write(mp, addr, data);
   } /* Sequential access device */
       
   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
   
   /* This setting come with fixed constant PAGESZ */
   int numfp = mp->maxsz / pagesz;
   struct framephy_struct *newfst, *fst;
   int iter = 0;

   if (numfp <= 0)
      return -1;

   pthread_mutex_lock(&memphy_mutex);
   /* Init head of free framephy list */
   fst = malloc(sizeof(struct framephy_struct));
   fst->fpn = iter;
   mp->free_fp_list = fst;

   /* We have list with first element, fill in the rest num-1 element member*/
   for (iter = 1; iter < numfp; iter++)
   {
      newfst = malloc(sizeof(struct framephy_struct));
      newfst->fpn = iter;
      newfst->fp_next = NULL;
      fst->fp_next = newfst;
      fst = newfst;
   }
   pthread_mutex_unlock(&memphy_mutex);
   return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
   pthread_mutex_lock(&memphy_mutex);
   struct framephy_struct *fp = mp->free_fp_list;

   if (fp == NULL) {
      pthread_mutex_unlock(&memphy_mutex);
      return -1;
   }
   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;

   /* MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   pthread_mutex_unlock(&memphy_mutex);
   free(fp);
   return 0;
}

int MEMPHY_dump(struct memphy_struct *mp)
{
   /*TODO dump memphy content mp->storage
    *     for tracing the memory content
    */
   //---------
   pthread_mutex_lock(&memphy_mutex);
   printf("EXCEPT ADDRESS HAS VALUE ZERO");
    printf("\n--------------------\n");
    char s1[] = "ADDRESS";
    char s2[] = "VALUE";
    printf("%6s | %6s\n", s1, s2);
    printf("--------------------\n");
    for (int i = 0; i < mp->maxsz; i++) {
        if (mp->storage[i] != 0)
            printf("%d: %d\n", i ,mp->storage[i]);
    }
   printf("--------------------\n");
   printf("\n");
   pthread_mutex_unlock(&memphy_mutex);
   return 0;
}

int MEMPHY_clean_frame(struct memphy_struct *mp, int fpn)
{
   pthread_mutex_lock(&memphy_mutex);
#if SYNC_MM
	pthread_mutex_lock(&mp->lock_storage);
#endif // SYNC_MM	
	memset(mp->storage + fpn * PAGING_PAGESZ, 0, PAGING_PAGESZ);
#if SYNC_MM
	pthread_mutex_unlock(&mp->lock_storage);
#endif // SYNC_MM
   pthread_mutex_unlock(&memphy_mutex);
	return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
   pthread_mutex_lock(&memphy_mutex);
   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   struct framephy_struct *up = mp->used_fp_list;
   struct framephy_struct *prev = NULL;

   while (up != NULL) {
      if (up->fpn == fpn) {
         if (prev == NULL) {
            mp->used_fp_list = up->fp_next;
         } else {
            prev->fp_next = up->fp_next;
         }
      }
      prev = up;
      up = up->fp_next;
   }

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;
   pthread_mutex_unlock(&memphy_mutex);
   return 0;
}

int MEMPHY_put_usedfp(struct memphy_struct *mp, int fpn) {
   //pthread_mutex_lock(&memphy_mutex);

   struct framephy_struct *fp = mp->used_fp_list;

   while (fp) {
      if (fp->fpn == fpn) {
         return 0;
      }
      fp = fp->fp_next;
   }
   fp = mp->used_fp_list;

   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->used_fp_list = newnode;

   //pthread_mutex_unlock(&memphy_mutex);
   return 0;
}

/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
   mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
   mp->maxsz = max_size;
   mp->free_fp_list = NULL;
   mp->used_fp_list = NULL;
   MEMPHY_format(mp, PAGING_PAGESZ);
   pthread_mutex_init(&memphy_mutex, NULL);
   mp->rdmflg = (randomflg != 0) ? 1 : 0;

   if (!mp->rdmflg) /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;

   return 0;
}

// #endif
