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
#include <math.h>

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
  if(reflectionDepth<=0){
    colour.r = 0.0f;
    colour.g = 0.0f;
    colour.b = 0.0f;
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

    if(best_hit.what->material->refractionIndex > 0.0f){ //Calculate refraction
      Ray refraction_ray;
      float refractionIndex;

      if(ray.inObject){
        refractionIndex = 1 / best_hit.what->material->refractionIndex;
      }
      else
      {
        refractionIndex = best_hit.what->material->refractionIndex;
      }

      double cosI = best_hit.normal.dot(ray.direction);
      double cosT = sqrt(1-(1/pow(refractionIndex,2))*(1-pow(cosI,2)));

      if(pow(cosT,2)>=0.0f){

        //calculate refraction ray direction
        refraction_ray.direction.x = ((1/refractionIndex) * ray.direction.x) - ((cosT - (1/refractionIndex)*cosI) * best_hit.normal.x); 
        refraction_ray.direction.y = ((1/refractionIndex) * ray.direction.y) - ((cosT - (1/refractionIndex)*cosI) * best_hit.normal.y); 
        refraction_ray.direction.z = ((1/refractionIndex) * ray.direction.z) - ((cosT - (1/refractionIndex)*cosI) * best_hit.normal.z); 

        //calculate refraction hits
        refraction_ray.position.x = best_hit.position.x + (0.0001f * refraction_ray.direction.x);
        refraction_ray.position.y = best_hit.position.y + (0.0001f * refraction_ray.direction.y);
        refraction_ray.position.z = best_hit.position.z + (0.0001f * refraction_ray.direction.z);
        refraction_ray.inObject = !ray.inObject;

        Colour refractionColour = raytrace(refraction_ray, objects, lights, depth, reflectionDepth-1);

        //calculate Fresno values
        double rpar = ((refractionIndex*fabsf(cosI))- cosT) / ((refractionIndex*fabsf(cosI)) + cosT);
        double rper = (fabsf(cosI) - (refractionIndex*cosT)) / (fabsf(cosI) - (refractionIndex*cosT));

        double kr = 0.5 * (pow(rpar,2) + pow(rper,2));
        double kt = 1-kr;

        best_hit.what->material->reflection = kr;
        best_hit.what->material->refraction = kt; 

        colour.r += refractionColour.r * best_hit.what->material->refraction;
        colour.g += refractionColour.g * best_hit.what->material->refraction;
        colour.b += refractionColour.b * best_hit.what->material->refraction;
      }
      else
      {
        best_hit.what->material->reflection = 1;
      }
    }

    if(best_hit.what->material->reflection > 0.0 ) //Calculate reflection
    {
      Ray reflection_ray; 
      best_hit.normal.reflection(ray.direction, reflection_ray.direction);
      reflection_ray.position.x = best_hit.position.x + (0.005f * reflection_ray.direction.x);
      reflection_ray.position.y = best_hit.position.y + (0.005f * reflection_ray.direction.y);
      reflection_ray.position.z = best_hit.position.z + (0.005f * reflection_ray.direction.z);	
     
      Colour reflectionColour = raytrace(reflection_ray, objects, lights, depth, reflectionDepth-1);
      colour.r += reflectionColour.r * best_hit.what->material->reflection;
      colour.g += reflectionColour.g * best_hit.what->material->reflection;
      colour.b += reflectionColour.b * best_hit.what->material->reflection;
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
  bool depthofField = true;
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
  v.x = -3.0f;
  v.y = 1.0f;
  v.z = 10.0f;
  
  Sphere *sphere = new Sphere(v, 1.0f);

  Vertex v2;
  v2.x = 3.0f;
  v2.y = 1.0f;
  v2.z = 5.0f;
  
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
	bp1.specular.r = 0.2f;
	bp1.specular.g = 0.2f;
	bp1.specular.b = 0.2f;
	bp1.power = 40.0f;
  bp1.reflection = 0.4f;
  bp1.refraction = 0.0f;
  bp1.refractionIndex = 0.0f;

	pm->material = &bp1;

  Phong bp2;

  bp2.ambient.r = 0.0f;
	bp2.ambient.g = 0.1f;
	bp2.ambient.b = 0.0f;
	bp2.diffuse.r = 0.0f;
	bp2.diffuse.g = 0.1f;
	bp2.diffuse.b = 0.0f;
	bp2.specular.r = 0.1f;
	bp2.specular.g = 0.1f;
	bp2.specular.b = 0.1f;
	bp2.power = 40.0f;
  bp2.refractionIndex=1.52f;

	sphere->material = &bp2;
 	sphere2->material = &bp2;

	pm->next = sphere;
  sphere->next = sphere2;
 
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
      ray.inObject = false; 

      if(depthofField)
      {
        //calculating focal point
        float focalLength = 5;
        //using focal length to calculate point on focal plane
        float t = sqrt((focalLength*focalLength)/((ray.direction.x*ray.direction.x) + (ray.direction.y*ray.direction.y) + (0.5*0.5)));
        Vertex focalPoint;
        focalPoint.x = ray.direction.x*t;
        focalPoint.y = ray.direction.y*t;
        focalPoint.z = ray.direction.z*t;

        float aperture = 20.0f;
        Colour colour;
        Colour sampledColour;
        float depth;
        for( int x = 0; x<=15; x++){
          //generating a random point on our aperture (circle)
          float angle = (rand() / RAND_MAX) * 2.0f * M_PI;
          float radius = rand() / RAND_MAX;
          float offsetX = cos(angle) * radius * aperture;
          float offsetY =  sin(angle) * radius * aperture;

          //calculating new focalRay 
          Ray focalRay;
          focalRay.position = ray.position; //might need to change this
          focalRay.direction.x = focalPoint.x - offsetX;
          focalRay.direction.y = focalPoint.y - offsetX;
          focalRay.direction.z = 0.5f;
          focalRay.direction.normalise();
          focalRay.inObject = false;

          sampledColour = raytrace(focalRay, pm, dl, depth, 5);
          colour.r += sampledColour.r;
          colour.g += sampledColour.g;
          colour.b += sampledColour.b;
        }
        fb->plotPixel(x, y, colour.r/16, colour.g/16, colour.b/16);
      }
      else
      {
      Colour colour;
      float depth;
      colour = raytrace(ray, pm, dl, depth, 3);
      fb->plotPixel(x, y, colour.r, colour.g, colour.b);
      fb->plotDepth(x,y, depth);
      }
    }

    cerr << "*" << flush;
  }
  
  // Output the framebuffer.
  fb->writeRGBFile((char *)"test.ppm");
  //  fb->writeDepthFile((char *)"depth.ppm");
  return 0;
  
}
