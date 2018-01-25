#include	"dthdr.h"

/*	Ordered set/multiset
**	dt:	dictionary being searched
**	obj:	the object to look for.
**	type:	search type.
**
**      Written by Kiem-Phong Vo (5/25/96)
*/

#if __STD_C
static Void_t* dttree(Dt_t* dt, Void_t* obj, int type)
#else
static Void_t* dttree(dt,obj,type)
Dt_t*		dt;
Void_t* 	obj;
int		type;
#endif
{
	Dtlink_t	*root, *t;
	int		cmp, lk, sz, ky;
	Void_t		*o, *k, *key;
	Dtlink_t	*l, *r, *me, link;
	int		n, minp, turn[DT_MINP];
	Dtcompar_f	cmpf;
	Dtdisc_t*	disc;

	UNFLATTEN(dt);
	disc = dt->disc; _DTDSC(disc,ky,sz,lk,cmpf);
	dt->type &= ~DT_FOUND;

	root = dt->data->here;
	if(!obj)
	{	if(!root || !(type&(DT_CLEAR|DT_FIRST|DT_LAST)) )
			return NIL(Void_t*);

		if(type&DT_CLEAR) /* delete all objects */
		{	if(disc->freef || disc->link < 0)
			{	do
				{	while((t = root->left) )
						RROTATE(root,t);
					t = root->right;
					if(disc->freef)
						(*disc->freef)(dt,_DTOBJ(root,lk),disc);
					if(disc->link < 0)
						(*dt->memoryf)(dt,(Void_t*)root,0,disc);
				} while((root = t) );
			}

			dt->data->size = 0;
			dt->data->here = NIL(Dtlink_t*);
			return NIL(Void_t*);
		}
		else /* computing largest/smallest element */
		{	if(type&DT_LAST)
			{	while((t = root->right) )
					LROTATE(root,t);
			}
			else /* type&DT_FIRST */
			{	while((t = root->left) )
					RROTATE(root,t);
			}

			dt->data->here = root;
			return _DTOBJ(root,lk);
		}
	}

	/* note that link.right is LEFT tree and link.left is RIGHT tree */
	l = r = &link;

	/* allow apps to delete an object "actually" in the dictionary */
	if(dt->meth->type == DT_OBAG && (type&(DT_DELETE|DT_DETACH)) )
	{	key = _DTKEY(obj,ky,sz);
		for(o = dtsearch(dt,obj); o; o = dtnext(dt,o) )
		{	k = _DTKEY(o,ky,sz);
			if(_DTCMP(dt,key,k,disc,cmpf,sz) != 0)
				break;
			if(o == obj)
			{	root = dt->data->here;
				l->right = root->left;
				r->left  = root->right;
				goto dt_delete;
			}
		}
	}

	if(type&(DT_MATCH|DT_SEARCH|DT_INSERT|DT_ATTACH))
	{	key = (type&DT_MATCH) ? obj : _DTKEY(obj,ky,sz);
		if(root)
			goto do_search;
	}
	else if(type&DT_RENEW)
	{	me = (Dtlink_t*)obj;
		obj = _DTOBJ(me,lk);
		key = _DTKEY(obj,ky,sz);
		if(root)
			goto do_search;
	}
	else if(root && _DTOBJ(root,lk) != obj)
	{	key = _DTKEY(obj,ky,sz);
	do_search:
		if(dt->meth->type == DT_OSET &&
		   (minp = dt->data->minp) != 0 && (type&(DT_MATCH|DT_SEARCH)) )
		{	/* simple search, note that minp should be even */
			for(t = root, n = 0; n < minp; ++n)
			{	k = _DTOBJ(t,lk); k = _DTKEY(k,ky,sz);
				if((cmp = _DTCMP(dt,key,k,disc,cmpf,sz)) == 0)
					return _DTOBJ(t,lk);
				else
				{	turn[n] = cmp;	
					if(!(t = cmp < 0 ? t->left : t->right) )
						return NIL(Void_t*);
				}
			}

			/* exceed search length, top-down splay now */
			for(n = 0; n < minp; n += 2)
			{	if(turn[n] < 0)
				{	t = root->left;
					if(turn[n+1] < 0)
					{	rrotate(root,t);
						rlink(r,t);
						root = t->left;
					}
					else
					{	llink(l,t);
						rlink(r,root);
						root = t->right;
					}
				}
				else
				{	t = root->right;
					if(turn[n+1] > 0)
					{	lrotate(root,t);
						llink(l,t);
						root = t->right;
					}
					else
					{	rlink(r,t);
						llink(l,root);
						root = t->left;
					}
				}
			}
		}

		while(1)
		{	k = _DTOBJ(root,lk); k = _DTKEY(k,ky,sz);
			if((cmp = _DTCMP(dt,key,k,disc,cmpf,sz)) == 0)
				break;
			else if(cmp < 0)
			{	if((t = root->left) )
				{	k = _DTOBJ(t,lk); k = _DTKEY(k,ky,sz);
					if((cmp = _DTCMP(dt,key,k,disc,cmpf,sz)) < 0)
					{	rrotate(root,t);
						rlink(r,t);
						if(!(root = t->left) )
							break;
					}
					else if(cmp == 0)
					{	rlink(r,root);
						root = t;
						break;
					}
					else /* if(cmp > 0) */
					{	llink(l,t);
						rlink(r,root);
						if(!(root = t->right) )
							break;
					}
				}
				else
				{	rlink(r,root);
					root = NIL(Dtlink_t*);
					break;
				}
			}
			else /* if(cmp > 0) */
			{	if((t = root->right) )
				{	k = _DTOBJ(t,lk); k = _DTKEY(k,ky,sz);
					if((cmp = _DTCMP(dt,key,k,disc,cmpf,sz)) > 0)
					{	lrotate(root,t);
						llink(l,t);
						if(!(root = t->right) )
							break;
					}
					else if(cmp == 0)
					{	llink(l,root);
						root = t;
						break;
					}
					else /* if(cmp < 0) */
					{	rlink(r,t);
						llink(l,root);
						if(!(root = t->left) )
							break;
					}
				}
				else
				{	llink(l,root);
					root = NIL(Dtlink_t*);
					break;
				}
			}
		}
	}

	if(root)
	{	/* found it, now isolate it */
		dt->type |= DT_FOUND;
		l->right = root->left;
		r->left = root->right;

		if(type&(DT_SEARCH|DT_MATCH))
		{ has_root:
			root->left = link.right;
			root->right = link.left;
			if((dt->meth->type&DT_OBAG) && (type&(DT_SEARCH|DT_MATCH)) )
			{	key = _DTOBJ(root,lk); key = _DTKEY(key,ky,sz);
				while((t = root->left) )
				{	/* find max of left subtree */
					while((r = t->right) )
						LROTATE(t,r);
					root->left = t;

					/* now see if it's in the same group */
					k = _DTOBJ(t,lk); k = _DTKEY(k,ky,sz);
					if(_DTCMP(dt,key,k,disc,cmpf,sz) != 0)
						break;
					RROTATE(root,t);
				}
			}
			dt->data->here = root;
			return _DTOBJ(root,lk);
		}
		else if(type&DT_NEXT)
		{	root->left = link.right;
			root->right = NIL(Dtlink_t*);
			link.right = root;
		dt_next:
			if((root = link.left) )	
			{	while((t = root->left) )
					RROTATE(root,t);
				link.left = root->right;
				goto has_root;
			}
			else	goto no_root;
		}
		else if(type&DT_PREV)
		{	root->right = link.left;
			root->left = NIL(Dtlink_t*);
			link.left = root;
		dt_prev:
			if((root = link.right) )
			{	while((t = root->right) )
					LROTATE(root,t);
				link.right = root->left;
				goto has_root;
			}
			else	goto no_root;
		}
		else if(type&(DT_DELETE|DT_DETACH))
		{	/* taking an object out of the dictionary */
		dt_delete:
			obj = _DTOBJ(root,lk);
			if(disc->freef && (type&DT_DELETE))
				(*disc->freef)(dt,obj,disc);
			if(disc->link < 0)
				(*dt->memoryf)(dt,(Void_t*)root,0,disc);
			if((dt->data->size -= 1) < 0)
				dt->data->size = -1;
			goto no_root;
		}
		else if(type&(DT_INSERT|DT_ATTACH))
		{	if(dt->meth->type&DT_OSET)
				goto has_root;
			else
			{	root->left = NIL(Dtlink_t*);
				root->right = link.left;
				link.left = root;
				goto dt_insert;
			}
		}
		else if(type&DT_RENEW) /* a duplicate */
		{	if(dt->meth->type&DT_OSET)
			{	if(disc->freef)
					(*disc->freef)(dt,obj,disc);
				if(disc->link < 0)
					(*dt->memoryf)(dt,(Void_t*)me,0,disc);
			}
			else
			{	me->left = NIL(Dtlink_t*);
				me->right = link.left;
				link.left = me;
				dt->data->size += 1;
			}
			goto has_root;
		}
	}
	else
	{	/* not found, finish up LEFT and RIGHT trees */
		r->left = NIL(Dtlink_t*);
		l->right = NIL(Dtlink_t*);

		if(type&DT_NEXT)
			goto dt_next;
		else if(type&DT_PREV)
			goto dt_prev;
		else if(type&(DT_SEARCH|DT_MATCH))
		{ no_root:
			while((t = r->left) )
				r = t;
			r->left = link.right;
			dt->data->here = link.left;
			return (type&DT_DELETE) ? obj : NIL(Void_t*);
		}
		else if(type&(DT_INSERT|DT_ATTACH))
		{ dt_insert:
			if(disc->makef && (type&DT_INSERT))
				obj = (*disc->makef)(dt,obj,disc);
			if(obj)
			{	if(lk >= 0)
					root = _DTLNK(obj,lk);
				else
				{	root = (Dtlink_t*)(*dt->memoryf)
						(dt,NIL(Void_t*),sizeof(Dthold_t),disc);
					if(root)
						((Dthold_t*)root)->obj = obj;
					else if(disc->makef && disc->freef &&
						(type&DT_INSERT))
						(*disc->freef)(dt,obj,disc);
				}
			}
			if(root)
			{	if(dt->data->size >= 0)
					dt->data->size += 1;
				goto has_root;
			}
			else	goto no_root;
		}
		else if(type&DT_RENEW)
		{	root = me;
			dt->data->size += 1;
			goto has_root;
		}
		else /*if(type&DT_DELETE)*/
		{	obj = NIL(Void_t*);
			goto no_root;
		}
	}

	return NIL(Void_t*);
}

/* make this method available */
static Dtmethod_t	_Dtoset =  { dttree, DT_OSET };
static Dtmethod_t	_Dtobag =  { dttree, DT_OBAG };
__DEFINE__(Dtmethod_t*,Dtoset,&_Dtoset);
__DEFINE__(Dtmethod_t*,Dtobag,&_Dtobag);

#ifndef KPVDEL /* backward compatibility - delete next time around */
Dtmethod_t		_Dttree = { dttree, DT_OSET };
__DEFINE__(Dtmethod_t*,Dtorder,&_Dttree);
__DEFINE__(Dtmethod_t*,Dttree,&_Dttree);
#endif

#ifdef NoF
NoF(dttree)
#endif
