/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2019.
 *
 * Do what you like with this code as long as you retain this comment.
 */

/* This is the entry point function for the program you need to create for lab two.
 * You should not need to modify this code.
 * It creates a framebuffer, loads an triangle mesh object, calls the drawing function to render the object and then outputs the framebuffer as a ppm file.
 *
 * On linux.bath.ac.uk:
 *
 * Compile the code using g++ -o lab2executable main_lab2.cpp framebuffer.cpp linedrawer.cpp polymesh.cpp -lm
 *
 * Execute the code using ./lab2executable
 *
 * This will produce an image file called test.ppm. You can convert this a png file for viewing using
 *
 * pbmropng test.ppm > test.png
 *
 * You are expected to fill in the missing code in polymesh.cpp.
 */

#include "framebuffer.h"
#include "ray.h"
#include "hit.h"
#include "polymesh.h"
#include "sphere.h"
#include "light.h"
#include "directional_light.h"
#include "material.h"
#include "phong.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

void object_test(Ray ray, Object *objects, Hit &best_hit)
{
  Object *obj = objects;

  best_hit.flag = false;

  while(obj != 0)
  {
    Hit obj_hit;
    obj_hit.flag=false;
	  
    obj->intersection(ray, obj_hit);

    
    if (obj_hit.flag)
    {
      if (obj_hit.t > 0.0f)
      {
        if (best_hit.flag == false)
        {
          best_hit = obj_hit;
        } 
        else if (obj_hit.t < best_hit.t)
        {
        best_hit = obj_hit;
        }
      }
    }  
    obj = obj->next;
  }
  return;
}

Colour raytrace(Ray ray, Object *objects, Light *lights, float depth, int reflectionDepth)
{
  Colour colour;
  std::cout<< reflectionDepth << "\n";
  if(reflectionDepth<=0){
    return colour;
  }
  // first step, find the closest primitive

  Hit shadow_hit;
  Hit best_hit;
  object_test(ray, objects, best_hit);

  // if we found a primitive then compute the colour we should see
  if(best_hit.flag)
  {
    best_hit.what->material->compute_base_colour(colour);
    depth = best_hit.t;
    Light *light = lights;

    while (light != (Light *)0)
    {
      Vector viewer;
      Vector ldir;

      viewer.x = -best_hit.position.x;
      viewer.y = -best_hit.position.y;
      viewer.z = -best_hit.position.z;
      viewer.normalise();

      bool lit;
      lit = light->get_direction(best_hit.position, ldir);

      if(ldir.dot(best_hit.normal)>0)
      {
	      lit=false;//light is facing wrong way.
      }

      if(lit)
      {
      
        Ray shadow_ray;

        shadow_ray.direction.x = -ldir.x;
        shadow_ray.direction.y = -ldir.y;
        shadow_ray.direction.z = -ldir.z;
        shadow_ray.position.x = best_hit.position.x + (0.0001f * shadow_ray.direction.x);
        shadow_ray.position.y = best_hit.position.y + (0.0001f * shadow_ray.direction.y);
        shadow_ray.position.z = best_hit.position.z + (0.0001f * shadow_ray.direction.z);

        object_test(shadow_ray, objects, shadow_hit);

        if(shadow_hit.flag==true)
        {
          if (shadow_hit.t < 1000000000.0f)
          {
            lit = false; //there's a shadow so no lighting, if realistically close
          }
        }
      }

      if (lit)
      {
        Colour intensity;
        Colour scaling;

        light->get_intensity(best_hit.position, scaling);

        best_hit.what->material->compute_light_colour(viewer, best_hit.normal, ldir, intensity);

        intensity.scale(scaling);

        colour.add(intensity);
      }

      light = light->next;
    }

    // TODO: compute reflection ray if material supports it.
    if(1) //do reflection for all materials at the moment, can change in future
    {
      Ray reflection_ray; 
      ray.direction.reflection(best_hit.normal, reflection_ray.direction);
      reflection_ray.position.x = best_hit.position.x + (0.0001f * reflection_ray.direction.x);
      reflection_ray.position.y = best_hit.position.y + (0.0001f * reflection_ray.direction.y);
      reflection_ray.position.z = best_hit.position.z + (0.0001f * reflection_ray.direction.z);	
     
      Colour reflectionColour = raytrace(reflection_ray, objects, lights, depth, reflectionDepth-1);
      colour.r += reflectionColour.r * best_hit.what->material->reflection.r;
      colour.g += reflectionColour.g * best_hit.what->material->reflection.g;
      colour.b += reflectionColour.b * best_hit.what->material->reflection.b;
    }

    // TODO: compute refraction ray if material supports it.
    if(1)
    {
    }

    return colour;
  } 
  else
  {
    depth = 7.0f;
    colour.r = 0.0f;
    colour.g = 0.0f;
    colour.b = 0.0f;
    return colour;
  }	
}

int main(int argc, char *argv[])
{
  int width = 512;
  int height = 512;
  // Create a framebuffer
  FrameBuffer *fb = new FrameBuffer(width,height);

  // The following transform allows 4D homogeneous coordinates to be transformed. It moves the supplied teapot model to somewhere visible.
  Transform *transform = new Transform(1.0f, 0.0f, 0.0f,  0.0f,
				       0.0f, 0.0f, 1.0f, -2.7f,
				       0.0f, 1.0f, 0.0f, 5.0f,
				       0.0f, 0.0f, 0.0f, 1.0f);

  //  Read in the teapot model.
  PolyMesh *pm = new PolyMesh((char *)"teapot_smaller.ply", transform);

  Vertex v;
  v.x = 0.0f;
  v.y = 0.0f;
  v.z = 5.0f;
  
  Sphere *sphere = new Sphere(v, 1.0f);

  Vertex v2;
  v2.x = -1.0f;
  v2.y = 1.0f;
  v2.z = 3.0f;
  
  Sphere *sphere2 = new Sphere(v2,0.5f);

  //  sphere->next = pm;

  Ray ray;

  ray.position.x = 0.0001f;
  ray.position.y = 0.0f;
  ray.position.z = 0.0f;

  DirectionalLight *dl = new DirectionalLight(Vector(1.01f, -1.0f, 1.0f),Colour(1.0f, 1.0f, 1.0f, 0.0f));

  Phong bp1;

	bp1.ambient.r = 0.2f;
	bp1.ambient.g = 0.0f;
	bp1.ambient.b = 0.0f;
	bp1.diffuse.r = 0.4f;
	bp1.diffuse.g = 0.0f;
	bp1.diffuse.b = 0.0f;
	bp1.specular.r = 0.4f;
	bp1.specular.g = 0.4f;
	bp1.specular.b = 0.4f;
	bp1.power = 40.0f;
  bp1.reflection.r = 0.4f;
  bp1.reflection.g = 0.4f;
  bp1.reflection.b = 0.4f;

	pm->material = &bp1;

  Phong bp2;

  bp2.ambient.r = 0.0f;
	bp2.ambient.g = 0.2f;
	bp2.ambient.b = 0.0f;
	bp2.diffuse.r = 0.0f;
	bp2.diffuse.g = 0.4f;
	bp2.diffuse.b = 0.0f;
	bp2.specular.r = 0.4f;
	bp2.specular.g = 0.4f;
	bp2.specular.b = 0.4f;
	bp2.power = 40.0f;
  bp2.reflection.r = 0.4f;
  bp2.reflection.g = 0.4f;
  bp2.reflection.b = 0.4f;

	sphere->material = &bp2;

 	sphere2->material = &bp2;

	pm->next = sphere2;

 
  for (int y = 0; y < height; y += 1)
  {
    for (int x = 0; x < width; x += 1)
    {
      float fx = (float)x/(float)width;
      float fy = (float)y/(float)height;

      Vector direction;

      ray.direction.x = (fx-0.5f);
      ray.direction.y = (0.5f-fy);
      ray.direction.z = 0.5f;
      ray.direction.normalise();

      Colour colour;
      float depth;
      int reflectionDepth = 3;

      colour = raytrace(ray, pm, dl, depth, reflectionDepth);

      fb->plotPixel(x, y, colour.r, colour.g, colour.b);
      fb->plotDepth(x,y, depth);
    }

    cerr << "*" << flush;
  }
  
  // Output the framebuffer.
  fb->writeRGBFile((char *)"test.ppm");
  //  fb->writeDepthFile((char *)"depth.ppm");
  return 0;
  
}
