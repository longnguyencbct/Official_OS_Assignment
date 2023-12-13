// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdlib.h>

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
#ifdef TDBG
  printf("MEMPHY_mv_csr\n");
#endif

  int numstep = 0;

  mp->cursor = 0;
  while (numstep < offset && numstep < mp->maxsz)
  {
    /* Traverse sequentially */
    mp->cursor = (mp->cursor + 1) % mp->maxsz;
    numstep++;
  }

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
#ifdef TDBG
  printf("MEMPHY_seq_read\n");
#endif

  if (mp == NULL)
    return -1;

  if (!mp->rdmflg)
    return -1; /* Not compatible mode for sequential read */

  MEMPHY_mv_csr(mp, addr);
  *value = (BYTE)mp->storage[addr];

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
#ifdef TDBG
  printf("MEMPHY_read\n");
#endif
  if (mp == NULL)
    return -1;

  if (mp->rdmflg)
    *value = mp->storage[addr];
  else /* Sequential access device */
    return MEMPHY_seq_read(mp, addr, value);

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
#ifdef TDBG
  printf("MEMPHY_seq_write\n");
#endif
  if (mp == NULL)
    return -1;

  if (!mp->rdmflg)
    return -1; /* Not compatible mode for sequential read */

  MEMPHY_mv_csr(mp, addr);
  mp->storage[addr] = value;

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
#ifdef TDBG
  printf("MEMPHY_write\n");
#endif
  if (mp == NULL)
    return -1;

  if (mp->rdmflg)
    mp->storage[addr] = data;
  else /* Sequential access device */
    return MEMPHY_seq_write(mp, addr, data);

  return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
#ifdef TDBG
  printf("MEMPHY_format\n");
#endif
  /* This setting come with fixed constant PAGESZ */
  int numfp = mp->maxsz / pagesz;
  struct framephy_struct *newfst, *fst;
  int iter = 0;

  if (numfp <= 0)
    return -1;

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

  return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
#ifdef TDBG
  printf("MEMPHY_get_freefp\n");
#endif
  struct framephy_struct *fp = mp->free_fp_list;

  if (fp == NULL)
    return -1;

  *retfpn = fp->fpn;
  // Changes here
  struct framephy_struct *fp_used = malloc(sizeof(struct framephy_struct));
  fp_used->fpn = fp->fpn;
  fp_used->fp_next = mp->used_fp_list;
  mp->used_fp_list = fp_used;
  // Changed above.
  mp->free_fp_list = fp->fp_next;

  /* MEMPHY is iteratively used up until its exhausted
   * No garbage collector acting then it not been released
   */

  free(fp);

  return 0;
}

int MEMPHY_dump(struct memphy_struct *mp)
{
// pthread_mutex_lock(&MEM_in_use);
#ifdef TDBG
  printf("MEMPHY_dump\n");
#endif
  /*TODO dump memphy contnt mp->storage
   *     for tracing the memory content
   */
  printf("\n");
  printf("Print content of RAM (only print nonzero value)\n");
  for (int i = 0; i < mp->maxsz; i++)
  {
    if (mp->storage[i] != 0)
    {
      printf("---------------------------------\n");
      printf("Address 0x%08x: %d\n", i, mp->storage[i]);
    }
  }
  printf("---------------------------------\n");
  // pthread_mutex_unlock(&MEM_in_use);
  return 0;
}
int RAM_dump(struct memphy_struct *mram)
{
  int freeCnt = 0;
  struct framephy_struct *fpit = mram->free_fp_list;
  while (fpit != NULL)
  {
    fpit = fpit->fp_next;
    freeCnt++;
  }
  printf("----------- RAM mapping status -----------\n");
  printf("Number of mapped frames:\t%d\n", mram->maxsz / PAGING_PAGESZ - freeCnt);
  printf("Number of remaining frames:\t%d\n", freeCnt);
  printf("------------------------------------------\n");
  return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
#ifdef TDBG
  printf("MEMPHY_put_freefp\n");
#endif
  struct framephy_struct *fp = mp->free_fp_list;
  struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

  /* Create new node with value fpn */
  newnode->fpn = fpn;
  newnode->fp_next = fp;
  mp->free_fp_list = newnode;

  return 0;
}
int MEMPHY_put_usedfp(struct memphy_struct *mp, int fpn)
{
  // pthread_mutex_lock(&memphy_mutex);

  struct framephy_struct *fp = mp->used_fp_list;

  while (fp)
  {
    if (fp->fpn == fpn)
    {
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

  // pthread_mutex_unlock(&memphy_mutex);
  return 0;
}

/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
#ifdef TDBG
  printf("init_memphy\n");
#endif
  mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
  mp->maxsz = max_size;

  MEMPHY_format(mp, PAGING_PAGESZ);

  mp->rdmflg = (randomflg != 0) ? 1 : 0;

  if (!mp->rdmflg) /* Not Ramdom acess device, then it serial device*/
    mp->cursor = 0;

  return 0;
}

// #endif
