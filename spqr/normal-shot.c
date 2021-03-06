/* $Id: normal-shot.c,v 1.42 2005/06/25 03:44:39 oohara Exp $ */

#include <stdio.h>
/* malloc, rand */
#include <stdlib.h>

#include "const.h"
#include "tenm_object.h"
#include "tenm_math.h"
#include "tenm_graphic.h"
#include "util.h"
#include "tenm_primitive.h"

#include "normal-shot.h"

#define NEAR_ZERO 0.0001

static int normal_shot_move(tenm_object *my, double turn_per_frame);
static int normal_shot_hit(tenm_object *my, tenm_object *your);
static int normal_shot_act(tenm_object *my, const tenm_object *player);
static int normal_shot_draw(tenm_object *my, int priority);

/* speed must be positive --- use normal_shot_new() instead if you want
 * a shot that doesn't move
 */
tenm_object *
normal_shot_angle_new(double x, double y, double speed, int theta,
                      int color)
{
  /* sanity check */
  if (speed < NEAR_ZERO)
  {
    fprintf(stderr, "normal_shot_angle_new: speed is non-positive (%f)\n",
            speed);
    return NULL;
  }

  return normal_shot_new(x, y,
                         speed * tenm_cos(theta),
                         speed * tenm_sin(theta),
                         color, -2, 0);
}

/* speed must be positive --- use normal_shot_new() instead if you want
 * a shot that doesn't move
 * if (x, y) and (target_x, target_y) are very close, shoot at a random
 * direction
 */
tenm_object *
normal_shot_point_new(double x, double y, double speed,
                      double target_x, double target_y,
                      int color)
{
  double dx;
  double dy;
  double temp;
  int temp_theta;
  
  /* sanity check */
  if (speed < NEAR_ZERO)
  {
    fprintf(stderr, "normal_shot_point_new: speed is non-positive (%f)",
            speed);
    return NULL;
  }

  dx = target_x - x;
  dy = target_y - y;
  temp = tenm_sqrt((int) (dx * dx + dy * dy));
  if (temp <= NEAR_ZERO)
  {
    /* shoot at a random direction */
    temp_theta = rand() % 360;
    dx = tenm_cos(temp_theta);
    dy = tenm_sin(temp_theta);
    temp = 1.0;
  }

  return normal_shot_new(x, y,
                         speed * dx / temp,
                         speed * dy / temp,
                         color, -2, 0);
}

/* list of count
 * [0] color
 * [1] life
 * [2] immortal time
 */
/* list of count_d
 * [0] speed x
 * [1] speed y
 */
/* if life > 0 when created, the shot disappears after _life frames
 * when immortal > 0, the shot does not disappear even if
 * it is out of the window
 */
tenm_object *
normal_shot_new(double x, double y, double speed_x, double speed_y,
                int color, int life, int immortal_time)
{
  tenm_primitive **p = NULL;
  tenm_object *new = NULL;
  int *count = NULL;
  double *count_d = NULL;

  /* sanity check */
  if (life == 0)
  {
    fprintf(stderr, "normal_shot_new: life is 0\n");
    return NULL;
  }

  p = (tenm_primitive **) malloc(sizeof(tenm_primitive *) * 1);
  if (p == NULL)
  {
    fprintf(stderr, "normal_shot_new: malloc(p) failed\n");
    return NULL;
  }

  p[0] = (tenm_primitive *) tenm_circle_new(x, y, 5.0);
  if (p[0] == NULL)
  {
    fprintf(stderr, "normal_shot_new: cannot set p[0]\n");
    free(p);
    return NULL;
  }

  count = (int *) malloc(sizeof(int) * 3);
  if (count == NULL)
  {
    fprintf(stderr, "normal_shot_new: malloc(count) failed\n");
    (p[0])->delete(p[0]);
    free(p);
    return NULL;
  }
  count_d = (double *) malloc(sizeof(double) * 2);
  if (count_d == NULL)
  {
    fprintf(stderr, "normal_shot_new: malloc(count_d) failed\n");
    free(count);
    (p[0])->delete(p[0]);
    free(p);
    return NULL;
  }

  count[0] = color;
  count[1] = life;
  count[2] = immortal_time;

  count_d[0] = speed_x;
  count_d[1] = speed_y;

  new = tenm_object_new("normal shot", ATTR_ENEMY_SHOT, ATTR_OPAQUE,
                        1, x, y,
                        3, count, 2, count_d, 1, p, 
                        (int (*)(tenm_object *, double)) (&normal_shot_move),
                        (int (*)(tenm_object *, tenm_object *))
                        (&normal_shot_hit),
                        (int (*)(tenm_object *, const tenm_object *))
                        (&normal_shot_act),
                        (int (*)(tenm_object *, int)) (&normal_shot_draw));
  if (new == NULL)
  {
    fprintf(stderr, "normal_shot_new: tenm_object_new failed\n");
    if (count_d != NULL)
      free(count_d);
    if (count != NULL)
      free(count);
    (p[0])->delete(p[0]);
    free(p);
    return NULL;
  }

  return new;
}

static int
normal_shot_move(tenm_object *my, double turn_per_frame)
{
  double dx_temp;
  double dy_temp;

  /* sanity check */
  if (my == NULL)
  {
    fprintf(stderr, "normal_shot_move: my is NULL\n");
    return 0;
  }
  if (turn_per_frame <= 0.5)
  {
    fprintf(stderr, "normal_shot_move: strange turn_per_frame (%f)\n",
            turn_per_frame);
    return 0;
  }
  
  dx_temp = my->count_d[0] / turn_per_frame;
  dy_temp = my->count_d[1] / turn_per_frame;
  my->x += dx_temp;
  my->y += dy_temp;
  tenm_move_mass(my->mass, dx_temp, dy_temp);

  if ((my->count[2] <= 0) && (!in_window_object(my)))
    return 1;

  return 0;
}

static int
normal_shot_hit(tenm_object *my, tenm_object *your)
{
  /* sanity check */
  if (my == NULL)
  {
    fprintf(stderr, "normal_shot_hit: my is NULL\n");
    return 0;
  }
  if (your == NULL)
  {
    fprintf(stderr, "normal_shot_hit: your is NULL\n");
    return 0;
  }

  if (your->attr & ATTR_OPAQUE)
    return 1;

  return 0;
}

static int
normal_shot_act(tenm_object *my, const tenm_object *player)
{
  /* sanity check */
  if (my == NULL)
  {
    fprintf(stderr, "normal_shot_act: my is NULL\n");
    return 0;
  }
  /* player == NULL is OK */

  if (my->count[1] > 0)
  {
    (my->count[1])--;
    if (my->count[1] <= 0)
      return 1;
  }

  if (my->count[2] > 0)
    (my->count[2])--;

  return 0;
}

static int
normal_shot_draw(tenm_object *my, int priority)
{
  int status = 0;
  tenm_color color;

  /* sanity check */
  if (my == NULL)
  {
    fprintf(stderr, "normal_shot_draw: my is NULL\n");
    return 0;
  }

  /* return if it is not my turn */
  if (priority != 1)
    return 0;

  /* value
   * green (#0, #1): 75
   * blue (#2, #3): 87
   * purple (#4, #5): 94
   */
  switch (my->count[0])
  {
  case 0:
    color = tenm_map_color(49, 191, 0);
    color = tenm_map_color(0, 223, 55);
    color = tenm_map_color(0, 191, 47);
    break;
  case 1:
    color = tenm_map_color(0, 223, 55);
    color = tenm_map_color(0, 191, 127);
    break;
  case 2:
    color = tenm_map_color(0, 191, 255);
    color = tenm_map_color(0, 167, 223);
    break;
  case 3:
    color = tenm_map_color(0, 127, 255);
    color = tenm_map_color(0, 111, 223);
    break;
  case 4:
    color = tenm_map_color(70, 0, 223);
    color = tenm_map_color(75, 0, 239);
    break;
  case 5:
    color = tenm_map_color(163, 0, 223);
    color = tenm_map_color(175, 0, 239);
    break;
  default:
    fprintf(stderr, "normal_shot_draw: strange my->count[0] (%d)\n",
            my->count[0]);
    color = tenm_map_color(0, 0, 0);
    break;
  }

  if (tenm_draw_circle((int) (my->x), (int) (my->y), 5, 2, color) != 0)
    status = 1;

  /* maybe useful for debugging */
  /*
  if (tenm_draw_primitive(my->mass->p[0], color) != 0)
    status = 1;
  */

  return status;
}
