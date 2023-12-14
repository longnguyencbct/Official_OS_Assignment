// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*------------------Bat dau phan lam----------------*/
void Update_LRU_lst(uint32_t *pte_rm)
{
  struct LRU_struct *p = lru_head;
  int fpn = PAGING_PTE_FPN(*pte_rm);
  while (p->lru_next != NULL)
  {
    if (p->fpn == fpn)
    {
      break;
    }
    p = p->lru_next;
  }
  if (p == lru_head)
  {
    if (lru_head == lru_tail)
    {
      return;
    }
    lru_head = lru_head->lru_next;
    lru_head->lru_pre = NULL;
    // free(p);
  }
  else if (p == lru_tail)
  {
    return;
  }
  else
  {
    p->lru_pre->lru_next = p->lru_next;
    p->lru_next->lru_pre = p->lru_pre;
    // free(p);
  }
  lru_tail->lru_next = p;
  p->lru_pre = lru_tail;
  p->lru_next = NULL;
  lru_tail = p;
  // p->pte = pte_rm;
  // lru_tail->lru_next = tmp;
  // tmp->lru_pre = lru_tail;
  // tmp->lru_next = NULL;
  // lru_tail = tmp;
}
void Add_LRU_page(uint32_t *pte_add)
{
  struct LRU_struct *tmp = malloc(sizeof(struct LRU_struct));
  tmp->pte = pte_add; 
  tmp->fpn = PAGING_PTE_FPN(*pte_add);
  // if (lru_tail != NULL)
  // {

  //   printf("TAIL: %08x\n", *lru_tail->pte);
  // }
  // printf("=======================================================Frame: %d\n", tmp->fpn);
  if (lru_head == NULL)
  {
    lru_head = tmp;
    lru_tail = lru_head;
    tmp->lru_pre = NULL;
    tmp->lru_next = NULL;
  }
  else
  {
    struct LRU_struct *p = lru_head;
    int flag = 0;
    while (p != NULL)
    {
      if (p->fpn == tmp->fpn)
      {
        flag = 1;
        printf("FLAG 1");
        break;
      }
      p = p->lru_next;
    }
    if (flag == 1)
    {
      if (p == lru_head)
      {
        if (lru_head == lru_tail)
        {
          return;
        }
        lru_head = lru_head->lru_next;
        lru_head->lru_pre = NULL;
        // free(p);
      }
      else if (p == lru_tail)
      {
        return;
      }
      else
      {
        p->lru_pre->lru_next = p->lru_next;
        p->lru_next->lru_pre = p->lru_pre;
        // free(p);
      }

      lru_tail->lru_next = p;
      p->lru_pre = lru_tail;
      p->lru_next = NULL;
      lru_tail = p;
      // lru_tail->pte = pte_add;
    }
    else
    {
      lru_tail->lru_next = tmp;
      tmp->lru_pre = lru_tail;
      tmp->lru_next = NULL;
      lru_tail = tmp;
      // lru_tail->pte = pte_add;
      return;
    }
  }
  // LRU_print_page();
}
// pthread_mutex_unlock(&FIFO_lock);

uint32_t *LRU_find_victim_page()
{
  // pthread_mutex_lock(&FIFO_lock);
  if (lru_head == NULL)
    return -1;
  struct LRU_struct *temp = lru_head;
  uint32_t *pte_res;
  pte_res = temp->pte;
  if (lru_head == lru_tail)
  {
    lru_head = lru_tail = NULL;
  }
  else
  {
    lru_head = lru_head->lru_next;

    if (lru_head != NULL)
    {
      lru_head->lru_pre = NULL;
    }

    temp->lru_next = NULL;
  }
  free(temp);
  // mutexlock here.
  // pthread_mutex_unlock(&FIFO_lock);
  return pte_res;
}

void LRU_print_page()
{
  // pthread_mutex_lock(&FIFO_lock);
  struct LRU_struct *temp = lru_head;
  printf("=================LRU LIST=================\n");
  printf("FPN: \n");
  if (temp == NULL)
    printf("\nEMPTY page directory\n");
  else
  {
    while (temp != NULL)
    {
      // printf("[%d][%08x]", PAGING_PTE_FPN(*(temp->pte)), *temp->pte);
      printf("[%d]", temp->fpn);
      if (temp->lru_next != NULL)
      {
        printf(" -> ");
      }
      temp = temp->lru_next;
    }
    printf("\n=======================================\n\n");
  }
  // pthread_mutex_lock(&FIFO_lock);
}

int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct rg_elmt)
{
  struct vm_rg_struct *rg_node = malloc(sizeof(struct vm_rg_struct));
  if (rg_elmt.rg_start >= rg_elmt.rg_end)
    return -1;

  rg_node->rg_start = rg_elmt.rg_start;
  rg_node->rg_end = rg_elmt.rg_end;
  // rg_node->valid = 0;
  rg_node->rg_next = mm->mmap->vm_freerg_list;
  /* Enlist the new region */
  struct vm_rg_struct *temp = mm->mmap->vm_freerg_list;
  while (temp->rg_next != NULL)
  {
    temp = temp->rg_next;
  }
  temp->rg_next = rg_node;
  rg_node->rg_next = NULL;
  return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
#ifdef TDBG
  printf("get_vma_by_num\n");
#endif
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = 0;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;
    vmait++;
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
#ifdef TDBG
  printf("get_symrg_byid\n");
#endif
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
#ifdef TDBG
  printf("__alloc\n");
#endif
  // LRU_print_page();
#ifdef RAM_STATUS_DUMP
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("Process %d ALLOC CALL | SIZE = %d\n", caller->pid, size);

#endif
  /*Allocate at the toproof */
  struct vm_rg_struct rgnode;
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
  {
    printf("Process %d alloc error: Invalid region\n", caller->pid);
    return -1;
  }
  else if (caller->mm->symrgtbl[rgid].rg_start > caller->mm->symrgtbl[rgid].rg_end)
  {
    printf("Process %d alloc error: Region was alloc before\n", caller->pid);
    return -1;
  }
  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    printf("\n>>>>>Alloc Case>>>>> GET FREE RG in FREERG LIST. #RGID: %d\n", rgid);
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

    *alloc_addr = rgnode.rg_start;
    int current_pgn = PAGING_PGN(rgnode.rg_start);
    uint32_t *current_pte = caller->mm->pgd[current_pgn];
    // struct framephy_struct *fpn_lst = caller->mram->used_fp_list;

    printf("current_pgn = %d\n", current_pgn);
    printf("print head: %08x\n", *lru_head->pte);
    printf("Looking for:\t %08x\n", caller->mm->pgd[current_pgn]);
    printf("Found:\t %08x\n", current_pte);

    // while (fpn_lst != NULL)
    // {
    //   pte_set_fpn(&current_pte, fpn_lst->fpn);
    //   if (caller->mm->pgd[current_pgn] == current_pte)
    //   {
    //     break;
    //   }

    //   current_pte = 0;
    //   fpn_lst = fpn_lst->fp_next;
    // }
    printf("\n>>>>>DONE>>>>>  #RGID: %d\n", rgid);

    // Add_LRU_page(&current_pte);
    Update_LRU_lst(&current_pte);
#ifdef RAM_STATUS_DUMP
    printf("FOUND A FREE region to alloc.\n");
    printf("=======================================\n");
    for (int it = 0; it < PAGING_MAX_SYMTBL_SZ; it++)
    {
      if (caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0)
        continue;
      else
        printf("Region id %d : start = %lu, end = %lu\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end);
    }
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
    printf("VMA id %d : start = %lu, end = %lu, sbrk = %lu\n", cur_vma->vm_id, cur_vma->vm_start, cur_vma->vm_end, cur_vma->sbrk);
    printf("=======================================\n");
    printf("Process %d Free Region list \n", caller->pid);
    struct vm_rg_struct *temp = caller->mm->mmap->vm_freerg_list;
    while (temp != NULL)
    {
      if (temp->rg_start != temp->rg_end)
        printf("Start = %lu, end = %lu\n", temp->rg_start, temp->rg_end);
      temp = temp->rg_next;
    }
    printf("=======================================\n");
    RAM_dump(caller->mram);
    // FIFO_printf_list();
    LRU_print_page();
#endif

    return 0;
  }

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
  printf("\n>>>>>Alloc Case>>>>>No free region. #RGID: %d\n", rgid);
  /*Attempt to increate limit to get space */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);
  // int inc_limit_ret
  int old_sbrk;

  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * inc_vma_limit(caller, vmaid, inc_sz)
   */
  inc_vma_limit(caller, vmaid, inc_sz);
  /*Successful increase limit */
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  *alloc_addr = old_sbrk;
  // Add free rg to region list
  struct vm_rg_struct *rgnode_temp = malloc(sizeof(struct vm_rg_struct));
  rgnode_temp->rg_start = old_sbrk + size;
  rgnode_temp->rg_end = cur_vma->sbrk;
  enlist_vm_freerg_list(caller->mm, *rgnode_temp); 

#ifdef RAM_STATUS_DUMP
  printf(">>>>>Done>>>>> #RGID: %d\n", rgid);
  printf("=======================================\n");
  for (int it = 0; it < PAGING_MAX_SYMTBL_SZ; it++)
  {
    if (caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0)
      continue;
    else
      printf("Region id %d : start = %lu, end = %lu\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end);
  }

  printf("=======================================\n");
  printf("Process %d Free Region list \n", caller->pid);
  struct vm_rg_struct *temp = caller->mm->mmap->vm_freerg_list;
  while (temp != NULL)
  {
    if (temp->rg_start != temp->rg_end)
      printf("Start = %lu, end = %lu\n", temp->rg_start, temp->rg_end);
    temp = temp->rg_next;
  }
  printf("=======================================\n");
  printf("VMA id %d : start = %lu, end = %lu, sbrk = %lu\n", cur_vma->vm_id, cur_vma->vm_start, cur_vma->vm_end, cur_vma->sbrk);
  RAM_dump(caller->mram);
  // FIFO_printf_list();
  LRU_print_page();
#endif
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
#ifdef TDBG
  printf("__free\n");
#endif
  struct vm_rg_struct *rgnode;

  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
  {
  LRU_print_page();
    printf("Process %d free error: Invalid region\n", caller->pid);
    return -1;
  }

  /* TODO: Manage the collect freed region to freerg_list */

  rgnode = get_symrg_byid(caller->mm, rgid);
  if (rgnode->rg_start == rgnode->rg_end)
  {
  LRU_print_page();
    printf("Process %d FREE Error: Region wasn't alloc or was freed before\n", caller->pid);
    return -1;
  }
  struct vm_rg_struct *rgnode_temp = malloc(sizeof(struct vm_rg_struct));
  // Clear content of region in RAM
  BYTE value;
  value = 0;
  for (int i = rgnode->rg_start; i < rgnode->rg_end; i++)
    pg_setval(caller->mm, i, value, caller);
  //(caller->mram,rgnode->rg_start,rgnode->rg_end)
  // Create new node for region
  rgnode_temp->rg_start = rgnode->rg_start;
  rgnode_temp->rg_end = rgnode->rg_end;
  rgnode->rg_start = rgnode->rg_end = 0;


  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->mm, *rgnode_temp);
#ifdef RAM_STATUS_DUMP
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("Process %d FREE CALL | Region id %d after free: [%lu,%lu]\n", caller->pid, rgid, rgnode->rg_start, rgnode->rg_end);
  for (int it = 0; it < PAGING_MAX_SYMTBL_SZ; it++)
  {
    if (caller->mm->symrgtbl[it].rg_start == 0 && caller->mm->symrgtbl[it].rg_end == 0)
      continue;
    else
      printf("Region id %d : start = %lu, end = %lu\n", it, caller->mm->symrgtbl[it].rg_start, caller->mm->symrgtbl[it].rg_end);
  }
  printf("=======================================\n");
  printf("Process %d Free Region list \n", caller->pid);
  struct vm_rg_struct *temp = caller->mm->mmap->vm_freerg_list;
  while (temp != NULL)
  {
    if (temp->rg_start != temp->rg_end)
      printf("Start = %lu, end = %lu\n", temp->rg_start, temp->rg_end);
    temp = temp->rg_next;
  }
  printf("=======================================\n");
#endif
LRU_print_page();
  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
#ifdef TDBG
  printf("pgalloc\n");
#endif
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
#ifdef TDBG
  printf("pgfree_data\n");
#endif
  return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
#ifdef TDBG
  printf("pg_getpage\n");
#endif
  // pthread_mutex_lock(&MEM_in_use);
  uint32_t pte = mm->pgd[pgn];
  if (!PAGING_PAGE_PRESENT(pte))
  { /* Page is not online, make it actively living */
    // int vicpgn, swpfpn;
    // uint32_t vicpte;
    // uint32_t vicpte
    // int vicfpn;
    // int tgtfpn = PAGING_SWP(pte);//the target frame storing our variable
    /* TODO: Play with your paging theory here */
    int fpn_temp = -1;
    if (MEMPHY_get_freefp(caller->mram, &fpn_temp) == 0)
    {

      // lay gia tri tgtfpn
      int tgtfpn = GETVAL(pte, GENMASK(20, 0), 5);

      // Copy frame from SWAP to RAM
      __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, fpn_temp);
      // Cap nhat gia tri pte
      pte_set_fpn(&mm->pgd[pgn], fpn_temp);

      // printf("DEBUG GIA: pte = %08x", mm->pgd[pgn]);
      Add_LRU_page(&mm->pgd[pgn]);
    }
    else
    {
      int tgtfpn = GETVAL(pte, GENMASK(20, 0), 5);
      // printf("DEBUG GIA: pte = %08x\n", mm->pgd[pgn]);
      int vicfpn, swpfpn;
      uint32_t *vicpte;
      /* Find pointer to pte of victim frame*/
      vicpte = LRU_find_victim_page();

      /* Variable for value of pte*/
      uint32_t vicpte_temp = *vicpte;
      /*Get victim frame*/
      vicfpn = GETVAL(vicpte_temp, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT); // 8191 in decimal is 0->12 bit =1 in binary (total 13bit)
      /* Get free frame in MEMSWP */
      if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) < 0)
      {
        printf("Out of SWAP");
        return -3000;
      }
#ifdef RAM_STATUS_DUMP
      printf("[Page Replacement]\tPID #%d:\tVictim:%d\tPTE:%08x\tTarget:%d\t\n", caller->pid, vicfpn, *vicpte, tgtfpn);
#endif
      /* Copy victim frame to swap */
      __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);

      /* Copy target frame from swap to mem */
      __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);

      // Cap nhat cho pte tro den page vua bi thay rang du lieu do da chuyen vao SWAP
      pte_set_swap(vicpte, 0, swpfpn);

      // Cap nhat gia tri frame number moi (trong Ram) cho page entry (bao rang pte da co frame number moi)
      pte_set_fpn(&mm->pgd[pgn], vicfpn);

#ifdef RAM_STATUS_DUMP
      printf("[After Swap]\tPID #%d:\tVictim:%d\tPTE:%08x\tTarget:%d\t\n", caller->pid, swpfpn, *vicpte, vicfpn);
#endif
      Add_LRU_page(&mm->pgd[pgn]);

      // Put frame trong trong swap vao free frame list
      MEMPHY_put_freefp(caller->active_mswp, tgtfpn);
    }

  }
  *fpn = GETVAL(mm->pgd[pgn], PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
  // pthread_mutex_unlock(&MEM_in_use);
  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
#ifdef TDBG
  printf("pg_getval\n");
#endif
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram, phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
#ifdef TDBG
  printf("pg_setval\n");
#endif
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_write(caller->mram, phyaddr, value);

  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
#ifdef TDBG
  printf("__read\n");
#endif
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
  {
    printf("Process %d read error: Invalid region\n", caller->pid);
    return -1;
  }

  /*------------------Bat dau phan lam----------------*/
  if (currg->rg_start >= currg->rg_end)
  {
    printf("Process %d read error: Region not found (freed or unintialized)\n", caller->pid);
    return -1;
  }
  else if (currg->rg_start + offset >= currg->rg_end || offset < 0)
  {
    printf("Process %d read error: Invalid offset when read!\n", caller->pid);
    return -1;
  }
  else
  {

    struct vm_rg_struct rgnode=*get_symrg_byid(caller->mm, rgid);
    int *alloc_addr = rgnode.rg_start;
    int current_pgn = PAGING_PGN(rgnode.rg_start);
    uint32_t *current_pte = caller->mm->pgd[current_pgn];

    Update_LRU_lst(&current_pte);
    pg_getval(caller->mm, currg->rg_start + offset, data, caller);
  }

  /*------------------Ket thuc phan lam---------------*/

  return 0;
}

/*pgread - PAGING-based read a region memory */
int pgread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    uint32_t offset,    // Source address = [source] + [offset]
    uint32_t destination)
{
#ifdef TDBG
  printf("pgread\n");
#endif
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  destination = (uint32_t)data;
#ifdef IODUMP
  if (val == 0){
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("Process %d read region=%d offset=%d value=%d\n", proc->pid, source, offset, data);
    }
  else
    printf("Process %d error when read region=%d offset=%d \n", proc->pid, source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif
#ifdef RAM_STATUS_DUMP
  LRU_print_page();
#endif
  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
#ifdef TDBG
  printf("__write\n");
#endif
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
  {
    printf("Process %d write error: Invalid region\n", caller->pid);
    return -1;
  }

  /*------------------Bat dau phan lam----------------*/
  if (currg->rg_start >= currg->rg_end)
  {
    printf("Process %d write error: Region not found (freed or unintialized)\n", caller->pid);
  }
  else if (currg->rg_start + offset >= currg->rg_end || offset < 0)
  {
    printf("Process %d write error: Invalid offset when write!\n", caller->pid);
    return -1;
  }
  else
  {

    struct vm_rg_struct rgnode=*get_symrg_byid(caller->mm, rgid);
    int *alloc_addr = rgnode.rg_start;
    int current_pgn = PAGING_PGN(rgnode.rg_start);
    uint32_t *current_pte = caller->mm->pgd[current_pgn];

    Update_LRU_lst(&current_pte);
    pg_setval(caller->mm, currg->rg_start + offset, value, caller);
  }
  /*------------------Ket thuc phan lam---------------*/

  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    uint32_t offset)
{
#ifdef TDBG
  printf("pgwrite\n");
#endif
#ifdef IODUMP
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("Process %d write region=%d offset=%d value=%d\n", proc->pid, destination, offset, data);
#endif
  int x = __write(proc, 0, destination, offset, data);
#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

#ifdef RAM_STATUS_DUMP
  LRU_print_page();
#endif
  return x;
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
#ifdef TDBG
  printf("free_pcb_memph\n");
#endif
  int pagenum, fpn;
  uint32_t pte;
  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte = caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    }
    else
    {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
#ifdef TDBG
  printf("get_vm_area_node_at_brk\n");
#endif
  struct vm_rg_struct *newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
// struct vm_area_struct *vma = caller->mm->mmap;
#ifdef TDBG
  printf("validate_overlap_vm_area\n");
#endif
  /* TODO validate the planned memory area is not overlapped */
  struct vm_area_struct *vmit = caller->mm->mmap;
  while (vmit != NULL)
  {
    if ((vmastart < vmit->vm_start && vmaend > vmit->vm_start))
    {
      printf("vm area overlap\n");
      return -1;
    }
    vmit = vmit->vm_next;
  }
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
#ifdef TDBG
  printf("inc_vma_limit\n");
#endif
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
    return -1; /*Overlap and failed allocation */

  /* The obtained vm area (only)
   * now will be alloc real ram region */
  cur_vma->vm_end += inc_sz;
  cur_vma->sbrk += inc_sz; // nếu đã tăng vma lên 1 bậc, thì tăng luôn sbrk để alloc cái mới từ đó.
#ifdef TDBG
  printf("inc_vma_limit\n");
#endif
  if (vm_map_ram(caller, area->rg_start, area->rg_end,
                 old_end, incnumpage, newrg) < 0)
    return -1; /* Map the memory to MEMRAM */

  return 0;
}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
#ifdef TDBG
  printf("find_victim_page\n");
#endif
  struct pgn_t *pg = mm->fifo_pgn;

  /* TODO: Implement the theorical mechanism to find the victim page */

  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
#ifdef TDBG
  printf("get_free_vmrg_area\n");
#endif
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        {                                /*End of free list */
          rgit->rg_start = rgit->rg_end; // dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
      break;
    }
    else
    {
      rgit = rgit->rg_next; // Traverse next rg
    }
  }

  if (newrg->rg_start == -1) // new region not found
    return -1;

  return 0;
}

// #endif