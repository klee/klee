#include	"dthdr.h"

/*	Close a dictionary
**
**	Written by Kiem-Phong Vo (05/25/96)
*/
#if __STD_C
int dtclose(reg Dt_t* dt)
#else
int dtclose(dt)
reg Dt_t*	dt;
#endif
{
	Dtdisc_t	*disc;
	int		ev = 0;

	if(!dt || dt->nview > 0 ) /* can't close if being viewed */
		return -1;

	/* announce the close event to see if we should continue */
	disc = dt->disc;
	if(disc->eventf &&
	   (ev = (*disc->eventf)(dt,DT_CLOSE,NIL(Void_t*),disc)) < 0)
		return -1;

	if(dt->view)	/* turn off viewing */
		dtview(dt,NIL(Dt_t*));

	if(ev == 0) /* release all allocated data */
	{	(void)(*(dt->meth->searchf))(dt,NIL(Void_t*),DT_CLEAR);
		if(dtsize(dt) > 0)
			return -1;

		if(dt->data->ntab > 0)
			(*dt->memoryf)(dt,(Void_t*)dt->data->htab,0,disc);
		(*dt->memoryf)(dt,(Void_t*)dt->data,0,disc);
	}

	if(dt->type == DT_MALLOC)
		free((Void_t*)dt);
	else if(ev == 0 && dt->type == DT_MEMORYF)
		(*dt->memoryf)(dt, (Void_t*)dt, 0, disc);

	if(disc->eventf)
		(void)(*disc->eventf)(dt, DT_ENDCLOSE, NIL(Void_t*), disc);

	return 0;
}
