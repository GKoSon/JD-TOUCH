#ifndef _PERMI_LIST_H_
#define _PERMI_LIST_H_

#define     EMPTY_ID        0xFFFFFFFF

#include "stdint.h"

#define LIST_SUCCESS        (1)
#define FIND_NULL_ID        (-1)
#define LIST_FULL           (-2)

#define	PERMI_LIST_MAX			(20000)
#define	PERMI_LISD_INDEX_SIZE	(sizeof(int))
#define PERMI_LIST_SIZE         (sizeof(permiListType))

typedef enum
{
    LIST_INIT = 0,
    LIST_BLACK,
    LIST_WRITE,
}listStatusEnums;


typedef struct _permiList
{
    uint64_t    ID;
    uint32_t    time;
    uint8_t     status;
    uint8_t     temp[3];
}permiListType;

typedef struct _permi_list
{
    int32_t (*find) (uint64_t cardNumber, permiListType *list);
    int32_t (*add)  (permiListType *list);
    int32_t (*del)  ( uint64_t cardNumber);
    void    (*clear)(void);
    void    (*show) (void);
}permi_list_type;

extern permi_list_type permi;


uint32_t permiList_read_id( uint32_t index);
void permiList_read_data(uint32_t index, permiListType *list);
void permiList_del_index( uint32_t index);
int32_t periList_add(permiListType *list);
int32_t permiList_del( uint64_t cardNumber);
void permi_list_clear_all( void );
void permiList_clear_overdue( void );
#endif

