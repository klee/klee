#include	"dthdr.h"

/*	Get statistics of a dictionary
**
**	Written by Kiem-Phong Vo (5/25/96)
*/

#if __STD_C
static void dttstat(Dtstat_t* ds, Dtlink_t* root, int depth, int* level)
#else
static void dttstat(ds,root,depth,level)
Dtstat_t*	ds;
Dtlink_t*	root;
int		depth;
int*		level;
#endif
{
	if(root->left)
		dttstat(ds,root->left,depth+1,level);
	if(root->right)
		dttstat(ds,root->right,depth+1,level);
	if(depth > ds->dt_n)
		ds->dt_n = depth;
	if(level)
		level[depth] += 1;
}

#if __STD_C
static void dthstat(reg Dtdata_t* data, Dtstat_t* ds, reg int* count)
#else
static void dthstat(data, ds, count)
reg Dtdata_t*	data;
Dtstat_t*	ds;
reg int*	count;
#endif
{
	reg Dtlink_t*	t;
	reg int		n, h;

	for(h = data->ntab-1; h >= 0; --h)
	{	n = 0;
		for(t = data->htab[h]; t; t = t->right)
			n += 1;
		if(count)
			count[n] += 1;
		else if(n > 0)
		{	ds->dt_n += 1;
			if(n > ds->dt_max)
				ds->dt_max = n;
		}
	}
}

#if __STD_C
int dtstat(reg Dt_t* dt, Dtstat_t* ds, int all)
#else
int dtstat(dt, ds, all)
reg Dt_t*	dt;
Dtstat_t*	ds;
int		all;
#endif
{
	reg int		i;
	static int	*Count, Size;

	UNFLATTEN(dt);

	ds->dt_n = ds->dt_max = 0;
	ds->dt_count = NIL(int*);
	ds->dt_size = dtsize(dt);
	ds->dt_meth = dt->data->type&DT_METHODS;

	if(!all)
		return 0;

	if(dt->data->type&(DT_SET|DT_BAG))
	{	dthstat(dt->data,ds,NIL(int*));
		if(ds->dt_max+1 > Size)
		{	if(Size > 0)
				free(Count);
			if(!(Count = (int*)malloc((ds->dt_max+1)*sizeof(int))) )
				return -1;
			Size = ds->dt_max+1;
		}
		for(i = ds->dt_max; i >= 0; --i)
			Count[i] = 0;
		dthstat(dt->data,ds,Count);
	}
	else if(dt->data->type&(DT_OSET|DT_OBAG))
	{	if(dt->data->here)
		{	dttstat(ds,dt->data->here,0,NIL(int*));
			if(ds->dt_n+1 > Size)
			{	if(Size > 0)
					free(Count);
				if(!(Count = (int*)malloc((ds->dt_n+1)*sizeof(int))) )
					return -1;
				Size = ds->dt_n+1;
			}

			for(i = ds->dt_n; i >= 0; --i)
				Count[i] = 0;
			dttstat(ds,dt->data->here,0,Count);
			for(i = ds->dt_n; i >= 0; --i)
				if(Count[i] > ds->dt_max)
					ds->dt_max = Count[i];
		}
	}
	ds->dt_count = Count;

	return 0;
}
