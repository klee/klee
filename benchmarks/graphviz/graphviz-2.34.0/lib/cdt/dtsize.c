#include	"dthdr.h"

/*	Return the # of objects in the dictionary
**
**	Written by Kiem-Phong Vo (5/25/96)
*/

#if __STD_C
static int treecount(reg Dtlink_t* e)
#else
static int treecount(e)
reg Dtlink_t*	e;
#endif
{	return e ? treecount(e->left) + treecount(e->right) + 1 : 0;
}

#if __STD_C
int dtsize(Dt_t* dt)
#else
int dtsize(dt)
Dt_t*	dt;
#endif
{
	reg Dtlink_t*	t;
	reg int		size;

	UNFLATTEN(dt);

	if(dt->data->size < 0) /* !(dt->data->type&(DT_SET|DT_BAG)) */
	{	if(dt->data->type&(DT_OSET|DT_OBAG))
			dt->data->size = treecount(dt->data->here);
		else if(dt->data->type&(DT_LIST|DT_STACK|DT_QUEUE))
		{	for(size = 0, t = dt->data->head; t; t = t->right)
				size += 1;
			dt->data->size = size;
		}
	}

	return dt->data->size;
}
