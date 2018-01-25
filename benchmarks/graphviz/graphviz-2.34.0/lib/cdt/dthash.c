#include	"dthdr.h"

/*	Hash table.
**	dt:	dictionary
**	obj:	what to look for
**	type:	type of search
**
**      Written by Kiem-Phong Vo (05/25/96)
*/

/* resize the hash table */
#if __STD_C
static void dthtab(Dt_t* dt)
#else
static void dthtab(dt)
Dt_t*	dt;
#endif
{
	reg Dtlink_t	*t, *r, *p, **s, **hs, **is, **olds;
	int		n, k;

	if(dt->data->minp > 0 && dt->data->ntab > 0) /* fixed table size */
		return;
	dt->data->minp = 0;

	n = dt->data->ntab;
	if(dt->disc && dt->disc->eventf &&
	   (*dt->disc->eventf)(dt, DT_HASHSIZE, &n, dt->disc) > 0 )
	{	if(n < 0) /* fix table size */
		{	dt->data->minp = 1;
			if(dt->data->ntab > 0 )
				return;
		}
		else /* set a particular size */
		{	for(k = 2; k < n; k *= 2)
				;
			n = k;
		}
	}
	else	n = 0;

	/* compute new table size */
	if(n <= 0)
	{	if((n = dt->data->ntab) == 0)
			n = HSLOT;
		while(dt->data->size > HLOAD(n))
			n = HRESIZE(n);
	}
	if(n == dt->data->ntab)
		return;

	/* allocate new table */
	olds = dt->data->ntab == 0 ? NIL(Dtlink_t**) : dt->data->htab;
	if(!(s = (Dtlink_t**)(*dt->memoryf)(dt,olds,n*sizeof(Dtlink_t*),dt->disc)) )
		return;
	olds = s + dt->data->ntab;
	dt->data->htab = s;
	dt->data->ntab = n;

	/* rehash elements */
	for(hs = s+n-1; hs >= olds; --hs)
		*hs = NIL(Dtlink_t*);
	for(hs = s; hs < olds; ++hs)
	{	for(p = NIL(Dtlink_t*), t = *hs; t; t = r)
		{	r = t->right;
			if((is = s + HINDEX(n,t->hash)) == hs)
				p = t;
			else	/* move to a new chain */
			{	if(p)
					p->right = r;
				else	*hs = r;
				t->right = *is; *is = t;
			}
		}
	}
}

#if __STD_C
static Void_t* dthash(Dt_t* dt, reg Void_t* obj, int type)
#else
static Void_t* dthash(dt,obj,type)
Dt_t*		dt;
reg Void_t*	obj;
int		type;
#endif
{
	reg Dtlink_t	*t, *r, *p;
	reg Void_t	*k, *key;
	reg uint	hsh;
	reg int		lk, sz, ky;
	reg Dtcompar_f	cmpf;
	reg Dtdisc_t*	disc;
	reg Dtlink_t	**s, **ends;

	UNFLATTEN(dt);

	/* initialize discipline data */
	disc = dt->disc; _DTDSC(disc,ky,sz,lk,cmpf);
	dt->type &= ~DT_FOUND;

	if(!obj)
	{	if(type&(DT_NEXT|DT_PREV))
			goto end_walk;

		if(dt->data->size <= 0 || !(type&(DT_CLEAR|DT_FIRST|DT_LAST)) )
			return NIL(Void_t*);

		ends = (s = dt->data->htab) + dt->data->ntab;
		if(type&DT_CLEAR)
		{	/* clean out all objects */
			for(; s < ends; ++s)
			{	t = *s;
				*s = NIL(Dtlink_t*);
				if(!disc->freef && disc->link >= 0)
					continue;
				while(t)
				{	r = t->right;
					if(disc->freef)
						(*disc->freef)(dt,_DTOBJ(t,lk),disc);
					if(disc->link < 0)
						(*dt->memoryf)(dt,(Void_t*)t,0,disc);
					t = r;
				}
			}
			dt->data->here = NIL(Dtlink_t*);
			dt->data->size = 0;
			dt->data->loop = 0;
			return NIL(Void_t*);
		}
		else	/* computing the first/last object */
		{	t = NIL(Dtlink_t*);
			while(s < ends && !t )
				t = (type&DT_LAST) ? *--ends : *s++;
			if(t && (type&DT_LAST))
				for(; t->right; t = t->right)
					;

			dt->data->loop += 1;
			dt->data->here = t;
			return t ? _DTOBJ(t,lk) : NIL(Void_t*);
		}
	}

	/* allow apps to delete an object "actually" in the dictionary */
	if(dt->meth->type == DT_BAG && (type&(DT_DELETE|DT_DETACH)) )
	{	if(!dtsearch(dt,obj) )
			return NIL(Void_t*);

		s = dt->data->htab + HINDEX(dt->data->ntab,dt->data->here->hash);
		r = NIL(Dtlink_t*);
		for(p = NIL(Dtlink_t*), t = *s; t; p = t, t = t->right)
		{	if(_DTOBJ(t,lk) == obj) /* delete this specific object */
				goto do_delete;
			if(t == dt->data->here)
				r = p;
		}

		/* delete some matching object */
		p = r; t = dt->data->here;
		goto do_delete;
	}

	if(type&(DT_MATCH|DT_SEARCH|DT_INSERT|DT_ATTACH) )
	{	key = (type&DT_MATCH) ? obj : _DTKEY(obj,ky,sz);
		hsh = _DTHSH(dt,key,disc,sz);
		goto do_search;
	}
	else if(type&(DT_RENEW|DT_VSEARCH) )
	{	r = (Dtlink_t*)obj;
		obj = _DTOBJ(r,lk);
		key = _DTKEY(obj,ky,sz);
		hsh = r->hash;
		goto do_search;
	}
	else /*if(type&(DT_DELETE|DT_DETACH|DT_NEXT|DT_PREV))*/
	{	if((t = dt->data->here) && _DTOBJ(t,lk) == obj)
		{	hsh = t->hash;
			s = dt->data->htab + HINDEX(dt->data->ntab,hsh);
			p = NIL(Dtlink_t*);
		}
		else
		{	key = _DTKEY(obj,ky,sz);
			hsh = _DTHSH(dt,key,disc,sz);
		do_search:
			t = dt->data->ntab <= 0 ? NIL(Dtlink_t*) :
			 	*(s = dt->data->htab + HINDEX(dt->data->ntab,hsh));
			for(p = NIL(Dtlink_t*); t; p = t, t = t->right)
			{	if(hsh == t->hash)
				{	k = _DTOBJ(t,lk); k = _DTKEY(k,ky,sz);
					if(_DTCMP(dt,key,k,disc,cmpf,sz) == 0)
						break;
				}
			}
		}
	}

	if(t) /* found matching object */
		dt->type |= DT_FOUND;

	if(type&(DT_MATCH|DT_SEARCH|DT_VSEARCH))
	{	if(!t)
			return NIL(Void_t*);
		if(p && (dt->data->type&DT_SET) && dt->data->loop <= 0)
		{	/* move-to-front heuristic */
			p->right = t->right;
			t->right = *s;
			*s = t;
		}
		dt->data->here = t;
		return _DTOBJ(t,lk);
	}
	else if(type&(DT_INSERT|DT_ATTACH))
	{	if(t && (dt->data->type&DT_SET) )
		{	dt->data->here = t;
			return _DTOBJ(t,lk);
		}

		if(disc->makef && (type&DT_INSERT) &&
		   !(obj = (*disc->makef)(dt,obj,disc)) )
			return NIL(Void_t*);
		if(lk >= 0)
			r = _DTLNK(obj,lk);
		else
		{	r = (Dtlink_t*)(*dt->memoryf)
				(dt,NIL(Void_t*),sizeof(Dthold_t),disc);
			if(r)
				((Dthold_t*)r)->obj = obj;
			else
			{	if(disc->makef && disc->freef && (type&DT_INSERT))
					(*disc->freef)(dt,obj,disc);
				return NIL(Void_t*);
			}
		}
		r->hash = hsh;

		/* insert object */
	do_insert:
		if((dt->data->size += 1) > HLOAD(dt->data->ntab) && dt->data->loop <= 0 )
			dthtab(dt);
		if(dt->data->ntab == 0)
		{	dt->data->size -= 1;
			if(disc->freef && (type&DT_INSERT))
				(*disc->freef)(dt,obj,disc);
			if(disc->link < 0)
				(*disc->memoryf)(dt,(Void_t*)r,0,disc);
			return NIL(Void_t*);
		}
		s = dt->data->htab + HINDEX(dt->data->ntab,hsh);
		if(t)
		{	r->right = t->right;
			t->right = r;
		}
		else
		{	r->right = *s;
			*s = r;
		}
		dt->data->here = r;
		return obj;
	}
	else if(type&DT_NEXT)
	{	if(t && !(p = t->right) )
		{	for(ends = dt->data->htab+dt->data->ntab, s += 1; s < ends; ++s)
				if((p = *s) )
					break;
		}
		goto done_adj;
	}
	else if(type&DT_PREV)
	{	if(t && !p)
		{	if((p = *s) != t)
			{	while(p->right != t)
					p = p->right;
			}
			else
			{	p = NIL(Dtlink_t*);
				for(s -= 1, ends = dt->data->htab; s >= ends; --s)
				{	if((p = *s) )
					{	while(p->right)
							p = p->right;
						break;
					}
				}
			}
		}
	done_adj:
		if(!(dt->data->here = p) )
		{ end_walk:
			if((dt->data->loop -= 1) < 0)
				dt->data->loop = 0;
			if(dt->data->size > HLOAD(dt->data->ntab) && dt->data->loop <= 0)
				dthtab(dt);
			return NIL(Void_t*);
		}
		else
		{	dt->data->type |= DT_WALK;
			return _DTOBJ(p,lk);
		}
	}
	else if(type&DT_RENEW)
	{	if(!t || (dt->data->type&DT_BAG) )
			goto do_insert;
		else
		{	if(disc->freef)
				(*disc->freef)(dt,obj,disc);
			if(disc->link < 0)
				(*dt->memoryf)(dt,(Void_t*)r,0,disc);
			return t ? _DTOBJ(t,lk) : NIL(Void_t*);
		}
	}
	else /*if(type&(DT_DELETE|DT_DETACH))*/
	{	/* take an element out of the dictionary */
	do_delete:
		if(!t)
			return NIL(Void_t*);
		else if(p)
			p->right = t->right;
		else if((p = *s) == t)
			p = *s = t->right;
		else
		{	while(p->right != t)
				p = p->right;
			p->right = t->right;
		}
		obj = _DTOBJ(t,lk);
		dt->data->size -= 1;
		dt->data->here = p;
		if(disc->freef && (type&DT_DELETE))
			(*disc->freef)(dt,obj,disc);
		if(disc->link < 0)
			(*dt->memoryf)(dt,(Void_t*)t,0,disc);
		return obj;
	}
}

static Dtmethod_t	_Dtset = { dthash, DT_SET };
static Dtmethod_t	_Dtbag = { dthash, DT_BAG };
__DEFINE__(Dtmethod_t*,Dtset,&_Dtset);
__DEFINE__(Dtmethod_t*,Dtbag,&_Dtbag);

#ifndef KPVDEL	/* for backward compatibility - remove next time */
Dtmethod_t		_Dthash = { dthash, DT_SET };
__DEFINE__(Dtmethod_t*,Dthash,&_Dthash);
#endif

#ifdef NoF
NoF(dthash)
#endif
