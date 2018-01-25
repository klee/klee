#include	"dthdr.h"

/*	Change discipline.
**	dt :	dictionary
**	disc :	discipline
**
**	Written by Kiem-Phong Vo (5/26/96)
*/

#if __STD_C
static Void_t* dtmemory(Dt_t* dt,Void_t* addr,size_t size,Dtdisc_t* disc)
#else
static Void_t* dtmemory(dt, addr, size, disc)
Dt_t* 		dt;	/* dictionary			*/
Void_t* 	addr;	/* address to be manipulate	*/
size_t		size;	/* size to obtain		*/
Dtdisc_t* 	disc;	/* discipline			*/
#endif
{
	if(addr)
	{	if(size == 0)
		{	free(addr);
			return NIL(Void_t*);
		}
		else	return realloc(addr,size);
	}
	else	return size > 0 ? malloc(size) : NIL(Void_t*);
}

#if __STD_C
Dtdisc_t* dtdisc(Dt_t* dt, Dtdisc_t* disc, int type)
#else
Dtdisc_t* dtdisc(dt,disc,type)
Dt_t*		dt;
Dtdisc_t*	disc;
int		type;
#endif
{
	reg Dtsearch_f	searchf;
	reg Dtlink_t	*r, *t;
	reg char*	k;
	reg Dtdisc_t*	old;

	if(!(old = dt->disc) )	/* initialization call from dtopen() */
	{	dt->disc = disc;
		if(!(dt->memoryf = disc->memoryf) )
			dt->memoryf = dtmemory;
		return disc;
	}

	if(!disc)	/* only want to know current discipline */
		return old;

	searchf = dt->meth->searchf;

	UNFLATTEN(dt);

	if(old->eventf && (*old->eventf)(dt,DT_DISC,(Void_t*)disc,old) < 0)
		return NIL(Dtdisc_t*);

	dt->disc = disc;
	if(!(dt->memoryf = disc->memoryf) )
		dt->memoryf = dtmemory;

	if(dt->data->type&(DT_STACK|DT_QUEUE|DT_LIST))
		goto done;
	else if(dt->data->type&DT_BAG)
	{	if(type&DT_SAMEHASH)
			goto done;
		else	goto dt_renew;
	}
	else if(dt->data->type&(DT_SET|DT_BAG))
	{	if((type&DT_SAMEHASH) && (type&DT_SAMECMP))
			goto done;
		else	goto dt_renew;
	}
	else /*if(dt->data->type&(DT_OSET|DT_OBAG))*/
	{	if(type&DT_SAMECMP)
			goto done;
	dt_renew:
		r = dtflatten(dt);
		dt->data->type &= ~DT_FLATTEN;
		dt->data->here = NIL(Dtlink_t*);
		dt->data->size = 0;

		if(dt->data->type&(DT_SET|DT_BAG))
		{	reg Dtlink_t	**s, **ends;
			ends = (s = dt->data->htab) + dt->data->ntab;
			while(s < ends)
				*s++ = NIL(Dtlink_t*);
		}

		/* reinsert them */
		while(r)
		{	t = r->right;
			if(!(type&DT_SAMEHASH))	/* new hash value */
			{	k = (char*)_DTOBJ(r,disc->link);
				k = _DTKEY((Void_t*)k,disc->key,disc->size);
				r->hash = _DTHSH(dt,k,disc,disc->size);
			}
			(void)(*searchf)(dt,(Void_t*)r,DT_RENEW);
			r = t;
		}
	}

done:
	return old;
}
