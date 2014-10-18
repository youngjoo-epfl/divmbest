#include	<math.h>
#include	"csa_types.h"
#include	"csa_defs.h"

extern	char	*st_pop();
extern	rhs_ptr	deq_list();
extern	void	st_reset();
extern	stack	reached_nodes;
extern	rhs_ptr	*bucket;
extern	long	num_buckets;
extern	rhs_ptr	head_rhs_node, tail_rhs_node;
extern	double	epsilon;

extern	unsigned	p_refines, r_scans;
extern	unsigned	myclock(), p_refine_time;

int	dfs_visit(w)

register	rhs_ptr	w;

{
register	lr_aptr	a, a_stop;
lhs_ptr	v;
register	rhs_ptr	x;
register	double	p;

w->node_info.srchng = TRUE;
if (w->node_info.priced_in && (v = w->matched))
  {
  a_stop = (v+1)->priced_out;
  p = w->p - v->matched->c;
  for (a = v->first; a != a_stop; a++)
    if ((a != v->matched) && (p + a->c - (x = a->head)->p < 0.0))
      {
      if (x->node_info.srchng)
	return(0);
      if (!x->node_info.srched && !dfs_visit(x))
	return(0);
      }
  }
w->node_info.srchng = FALSE; w->node_info.srched = TRUE;
st_push(reached_nodes, w);

return(1);
}

int	top_sort()

{
register	rhs_ptr	w, w_stop;

st_reset(reached_nodes);
for (w = head_rhs_node; w != tail_rhs_node; w++)
  w->node_info.srched = w->node_info.srchng = FALSE;

w_stop = head_rhs_node - 1;
for (w--; w != w_stop; w--)
  if (!w->node_info.srched && !dfs_visit(w))
    return(0);
return(1);
}

/*
Doing an r_scan on w updates the current estimate of required price
changes on nodes adjacent (in the rhs sense) to w to establish
epsilon-optimality with no change to the matching.
*/

void	r_scan(w)

register	rhs_ptr	w;

{
register	lr_aptr	a, a_stop;
lhs_ptr	v;
register	rhs_ptr	x;
register	long	wk, xk;
register	double	p;
double	w_to_x_cost;

r_scans++;
if (w->node_info.priced_in && (v = w->matched))
  {
  a_stop = (v+1)->priced_out;
  p = w->p - v->matched->c;
  wk = w->key;
  for (a = v->first; a != a_stop; a++)
    if (a != v->matched)
      {
      if ((w_to_x_cost = p + a->c - (x = a->head)->p) < 0.0)
	xk = wk;
      else
	/*
	Preliminary check to avoid overflow and expensive cast operation.
	*/
	if ((epsilon * (wk - x->key)) > w_to_x_cost)
	  xk = wk - 1 - (long) (w_to_x_cost / epsilon);
	else
	  xk = x->key;
      if (xk > x->key)
	{
	delete_list(x, &bucket[x->key]);
	x->key = xk;
	insert_list(x, &bucket[xk]);
	}
      }
  }
w->p -= epsilon * w->key;
w->key = num_buckets;
}

int	p_refine()

{
register	rhs_ptr	w, x;
lhs_ptr	v;
lr_aptr	a, a_stop;
long	wk, xk, max_key = 0;
int	eps_opt = FALSE;
register	double	w_to_x_cost, p;

p_refine_time -= myclock();
p_refines++;

while (top_sort() && !eps_opt)
  {
  for (w = head_rhs_node; w != tail_rhs_node; w++)
    w->key = 0;

  max_key = 0;
  while (!st_empty(reached_nodes))
    {
    w = (rhs_ptr) st_pop(reached_nodes);
    wk = w->key;
    if (wk > max_key) max_key = wk;
    if ((v = w->matched) && w->node_info.priced_in)
      {
      a_stop = (v+1)->priced_out;
      p = w->p - v->matched->c;
      for (a = v->first; a != a_stop; a++)
	{
	x = a->head;
	if ((epsilon * (wk - x->key)) > (w_to_x_cost = p + a->c - x->p))
	  {
	  xk = wk - 1 - (long) floor(w_to_x_cost / epsilon);
	  if (xk > x->key) x->key = xk;
	  }
	}
      }
    }

  if (max_key == 0)
    eps_opt = TRUE;
  else
    {
    for (w = head_rhs_node; w != tail_rhs_node; w++)
      insert_list(w, &bucket[w->key]);
    for (; max_key > 0; max_key--)
      while (bucket[max_key] != tail_rhs_node)
	r_scan(deq_list(&bucket[max_key]));
    bucket[0] = tail_rhs_node;
    }
  }

p_refine_time += myclock();
return(eps_opt);
}
