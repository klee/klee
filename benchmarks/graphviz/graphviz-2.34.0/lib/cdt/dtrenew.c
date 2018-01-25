#include	"dthdr.h"


/*	Renew the object at the current finger.
**
**	Written by Kiem-Phong Vo (5/25/96)
*/

#if __STD_C
Void_t* dtrenew(Dt_t* dt, reg Void_t* obj)
#else
Void_t* dtrenew(dt, obj)
Dt_t*		dt;
reg Void_t*	obj;
#endif
{
	reg Void_t*	key;
	reg Dtlink_t	*e, *t, **s;
	reg Dtdisc_t*	disc = dt->disc;

	UNFLATTEN(dt);

	if(!(e = dt->data->here) || _DTOBJ(e,disc->link) != obj)
		return NIL(Void_t*);

	if(dt->data->type&(DT_STACK|DT_QUEUE|DT_LIST))
		return obj;
	else if(dt->data->type&(DT_OSET|DT_OBAG) )
	{	if(!e->right )	/* make left child the new root */
			dt->data->here = e->left;
		else		/* make right child the new root */
		{	dt->data->here = e->right;

			/* merge left subtree to right subtree */
			if(e->left)
			{	for(t = e->right; t->left; t = t->left)
					;
				t->left = e->left;
			}
		}
	}
	else /*if(dt->data->type&(DT_SET|DT_BAG))*/
	{	s = dt->data->htab + HINDEX(dt->data->ntab,e->hash);
		if((t = *s) == e)
			*s = e->right;
		else
		{	for(; t->right != e; t = t->right)
				;
			t->right = e->right;
		}
		key = _DTKEY(obj,disc->key,disc->size);
		e->hash = _DTHSH(dt,key,disc,disc->size);
		dt->data->here = NIL(Dtlink_t*);
	}

	dt->data->size -= 1;
	return (*dt->meth->searchf)(dt,(Void_t*)e,DT_RENEW) ? obj : NIL(Void_t*);
}
