/*
Name: Kyle Lukaszek
ID: 1113798

File: task.c
CIS 3090: Parallel Programming
Assignment 1
Due: 09/29/2023
*/

// Provided by professor
/* The original ray tracing code was taken from the web site:
    https://www.purplealienplanet.com/node/23 */

/* A simple ray tracer */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> /* Needed for boolean datatype */
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

	/* added scale and threads to allow command line controls */
	/* scale controls how many times the height and width will increase */
	/* threads is used to indicate how many threads should be created */
int scale = 1;
int threads = 1;
	/* output 0 means not output file is created by default */
	/* change to 1 to create output image file image.ppm */
	/* using the -o on the command line sets output==1 (creates the file) */
int output = 0;


#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

   /* Width and height of out image */
   /* scale added to allow larger output images to be created */
#define WIDTH  (800 * scale)
#define HEIGHT (600 * scale)

/* The vector structure */
typedef struct{
      float x,y,z;
}vector;

/* The sphere */
typedef struct{
        vector pos;
        float  radius;
   int material;
}sphere; 

/* The ray */
typedef struct{
        vector start;
        vector dir;
}ray;

/* Colour */
typedef struct{
   float red, green, blue;
}colour;

/* Material Definition */
typedef struct{
   colour diffuse;
   float reflection;
}material;

/* Lightsource definition */
typedef struct{
   vector pos;
   colour intensity;
}light;

/* Subtract two vectors and return the resulting vector */
vector vectorSub(vector *v1, vector *v2){
   vector result = {v1->x - v2->x, v1->y - v2->y, v1->z - v2->z };
   return result;
}

/* Multiply two vectors and return the resulting scalar (dot product) */
float vectorDot(vector *v1, vector *v2){
   return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

/* Calculate Vector x Scalar and return resulting Vector*/ 
vector vectorScale(float c, vector *v){
        vector result = {v->x * c, v->y * c, v->z * c };
        return result;
}

/* Add two vectors and return the resulting vector */
vector vectorAdd(vector *v1, vector *v2){
        vector result = {v1->x + v2->x, v1->y + v2->y, v1->z + v2->z };
        return result;
}

/* Check if the ray and sphere intersect */
bool intersectRaySphere(ray *r, sphere *s, float *t){
   
   bool retval = false;

   /* A = d.d, the vector dot product of the direction */
   float A = vectorDot(&r->dir, &r->dir); 
   
   /* We need a vector representing the distance between the start of 
    * the ray and the position of the circle.
    * This is the term (p0 - c) 
    */
   vector dist = vectorSub(&r->start, &s->pos);
   
   /* 2d.(p0 - c) */  
   float B = 2 * vectorDot(&r->dir, &dist);
   
   /* (p0 - c).(p0 - c) - r^2 */
   float C = vectorDot(&dist, &dist) - (s->radius * s->radius);
   
   /* Solving the discriminant */
   float discr = B * B - 4 * A * C;
   
   /* If the discriminant is negative, there are no real roots.
    * Return false in that case as the ray misses the sphere.
    * Return true in all other cases (can be one or two intersections)
    * t represents the distance between the start of the ray and
    * the point on the sphere where it intersects.
    */
   if(discr < 0)
      retval = false;
   else{
      float sqrtdiscr = sqrtf(discr);
      float t0 = (-B + sqrtdiscr)/(2);
      float t1 = (-B - sqrtdiscr)/(2);
      
      /* We want the closest one */
      if(t0 > t1)
         t0 = t1;

      /* Verify t1 larger than 0 and less than the original t */
      if((t0 > 0.001f) && (t0 < *t)){
         *t = t0;
         retval = true;
      }else
         retval = false;
   }

   return retval;
}

// Struct to hold the data for the determine_current_sphere function
typedef struct
{
   int thread_id;

   int start;
   int end;

   ray r;
   float* t;
   sphere* spheres;
   int* currentSphere;

   pthread_mutex_t* mutex;
} SphereThreadData;

//Function to determine the current closest sphere for a given ray using threads
//So much slower than the original code that it is not worth using threads but this is to be expected since the overhead of creating threads is greater than the time saved by using threads
void* determine_current_sphere(void *args)
{
   SphereThreadData *data = (SphereThreadData *) args;

   //printf("Thread %d: Current Sphere: %d\n", data->thread_id, *data->currentSphere);

   float local_t = 20000.0f;
   int localCurrentSphere = -1;
   unsigned int i;

   // Iterate over the spheres that this thread is responsible for
   for(i = data->start; i < data->end; i++){
      if(intersectRaySphere(&data->r, &data->spheres[i], &local_t))
      {
         if (localCurrentSphere == -1 || local_t < 20000.0f)
         {
            localCurrentSphere = i;
         }
      }
   }

   /*
   lock the mutex and update the current sphere if the local current sphere is not -1 and if the local current sphere has a greater value
   
   ray.c code used the largest value of i in a for loop that intersected with a sphere so that is why we check for a greater value

            unsigned int i;
            for(i = 0; i < 3; i++){
               if(intersectRaySphere(&r, &spheres[i], &t))
                  currentSphere = i;
            }

   set the value of the current sphere to the local current sphere and set the value of t to the local t

   */
   pthread_mutex_lock(data->mutex);
   if (localCurrentSphere != -1 && localCurrentSphere > *(data->currentSphere))
   {
      *(data->currentSphere) = localCurrentSphere;
      *(data->t) = local_t;
   }
   pthread_mutex_unlock(data->mutex);

   // Free the memory allocated for the thread data
   free(data);
}

// Struct to hold the data for the calculate_shadow pthread function
typedef struct
{
   int thread_id;
   int start;
   int end;

   float t;
   bool *inShadow;

   ray lightRay;
   sphere *spheres;
   pthread_mutex_t *mutex;

} ShadowThreadData;

// Function to calculate shadows at a given point using threads
void *calculate_shadow(void *args)
{
   ShadowThreadData *data = (ShadowThreadData *)args;
   bool inShadow = false;

   // Iterate over the spheres and check if the light ray intersects with any of them
   for (int i = data->start; i < data->end; i++)
   {
      if (intersectRaySphere(&data->lightRay, &data->spheres[i], &data->t))
      {
         inShadow = true;
         break;
      }
   }

   // If the point is in shadow, lock the mutex and update the value at the pointer for the current pixel
   if (inShadow)
   {
      // Store the shadow result in the shared array
      pthread_mutex_lock(data->mutex);
      *(data->inShadow) = inShadow;
      pthread_mutex_unlock(data->mutex);
   }

   // Free the memory allocated for the thread data
   free(data);
}

// Struct to hold the data for the calculate_light pthread function
typedef struct 
{
   int thread_id;
   int start;
   int end;

   float *red;
   float *green;
   float *blue;

   float coef;

   vector newStart;
   vector n;
   material currentMat;

   light *lights;
   sphere *spheres;

   pthread_mutex_t *mutex;

} LightThreadData;

// Function to calculate the light at a given point using threads
void* calculate_light(void *args)
{
   LightThreadData *data = (LightThreadData *) args;

   vector newStart = data->newStart;
   vector n = data->n;
   float coef = data->coef;
   light *lights = data->lights;
   sphere *spheres = data->spheres;
   material currentMat = data->currentMat;

   pthread_t* thread_handles = malloc(sizeof(pthread_t) * threads);

   // Iterate over the lights that this thread is responsible for and calculate the light for the current pixel (light is added to the rgb values for the current pixel)
   for (int i = data->start; i < data->end; i++)
   {
      light currentLight = lights[i];
      vector dist = vectorSub(&currentLight.pos, &newStart);
      if (vectorDot(&n, &dist) <= 0.0f)
         continue;
      float t = sqrtf(vectorDot(&dist, &dist));
      if (t <= 0.0f)
         continue;

      ray lightRay;
      lightRay.start = newStart;
      lightRay.dir = vectorScale((1 / t), &dist);

      /* Calculate shadows */
      bool inShadow = false;
      int num_spheres = sizeof(spheres) / sizeof(sphere);

      // Calculate the number of spheres per thread
      int spheresPerThread = num_spheres / threads;
      int extraSpheres = num_spheres % threads;

      // Set a minimum number of spheres per thread
      if (spheresPerThread == 0)
      {
         spheresPerThread = 1; // Minimum one sphere per thread
         extraSpheres = 0;     // No extra spheres in this case
      }

      // Create threads to calculate shadows
      for (int k = 0; k < threads; k++)
      {
         // In the event that there are more spheres than threads, we don't want to create more threads than there are spheres (in this case there are only 3 spheres but we could have more)
         if (k > num_spheres-1) break;

         int factor = (num_spheres / threads);
         if (threads > num_spheres || factor == 0) factor = 1;

         ShadowThreadData *data = malloc(sizeof(ShadowThreadData));
         data->thread_id = k;
         data->start = k * spheresPerThread;

         // If this is the last thread, it will be responsible for the extra spheres
         data->end = (k == (threads - 1)) ? (data->start + spheresPerThread + extraSpheres) : (data->start + spheresPerThread);

         data->t = t;
         data->inShadow = &inShadow;

         data->lightRay = lightRay;
         data->spheres = spheres;
         data->mutex = data->mutex;

         int result = pthread_create(&thread_handles[k], NULL, calculate_shadow, (void *) data);
         if (result != 0) {
            printf("Error creating thread %d to calculate shadow\n", k);
            free(thread_handles);
            exit(1);
         }
      }

      // Join the threads
      for (int k = 0; k < threads; k++)
      {
         // In the event that there are more spheres than threads, we don't want to create more threads than there are spheres (in this case there are only 3 spheres but we could have more)
         if (k > num_spheres-1) break;

         int result = pthread_join(thread_handles[k], NULL);
         if (result != 0) {
            printf("Error joining thread %d to calculate shadow\n", k);
            free(thread_handles);
            exit(1);
         }
      }

      // If the point is not in shadow, calculate the rgb light at the point
      if (!inShadow)
      {
         /* Lambert diffusion */
         float lambert = vectorDot(&lightRay.dir, &n) * coef;

         // Lock the mutex and update the rgb values at the pointer for the current pixel based on the lambert value, the current light intensity, and the current material diffuse value
         pthread_mutex_lock(data->mutex);
         *(data->red) += lambert * currentLight.intensity.red * currentMat.diffuse.red;
         *(data->green) += lambert * currentLight.intensity.green * currentMat.diffuse.green;
         *(data->blue) += lambert * currentLight.intensity.blue * currentMat.diffuse.blue;
         pthread_mutex_unlock(data->mutex);
      }
   }

   free(thread_handles);
   free(data);
}

/* Output data as PPM file */
void saveppm(char *filename, unsigned char *img, int width, int height){
   /* FILE pointer */
   FILE *f;

   /* Open file for writing */
   f = fopen(filename, "wb");

   /* PPM header info, including the size of the image */
   fprintf(f, "P6 %d %d %d\n", width, height, 255);

   /* Write the image data to the file - remember 3 byte per pixel */
   fwrite(img, 3, width*height, f);

   /* Make sure you close the file */
   fclose(f);
}

void readArgs(int argc, char *argv[]) {
int i;
   for (i=1; i<argc; i++) {
      if (strcmp(argv[i], "-s") == 0) {
         sscanf(argv[i+1], "%d", &scale);
      }
      if (strcmp(argv[i], "-t") == 0) {
         sscanf(argv[i+1], "%d", &threads);
      }
      if (strcmp(argv[i], "-o") == 0) {
         output = 1;
      }
   }
   printf("scale %d, threads %d, ", scale, threads);
   if (output == 0)
      printf("no output file created\n");
   else
      printf("output file image.ppm created\n");
}

int main(int argc, char *argv[]){

   ray r;

   readArgs(argc, argv);
   
   material materials[3];
   int num_materials = sizeof(materials) / sizeof(material);
   materials[0].diffuse.red = 1;
   materials[0].diffuse.green = 0;
   materials[0].diffuse.blue = 0;
   materials[0].reflection = 0.2;
   
   materials[1].diffuse.red = 0;
   materials[1].diffuse.green = 1;
   materials[1].diffuse.blue = 0;
   materials[1].reflection = 0.5;
   
   materials[2].diffuse.red = 0;
   materials[2].diffuse.green = 0;
   materials[2].diffuse.blue = 1;
   materials[2].reflection = 0.9;
   
   sphere spheres[3]; 
   int num_spheres = sizeof(spheres) / sizeof(sphere);
   spheres[0].pos.x = 200 * scale;
   spheres[0].pos.y = 300 * scale;
   spheres[0].pos.z = 0 * scale;
   spheres[0].radius = 100 * scale;
   spheres[0].material = 0;
   
   spheres[1].pos.x = 400 * scale;
   spheres[1].pos.y = 400 * scale;
   spheres[1].pos.z = 0 * scale;
   spheres[1].radius = 100 * scale;
   spheres[1].material = 1;
   
   spheres[2].pos.x = 500 * scale;
   spheres[2].pos.y = 140 * scale;
   spheres[2].pos.z = 0 * scale;
   spheres[2].radius = 100 * scale;
   spheres[2].material = 2;
   
   light lights[3];
   int num_lights = sizeof(lights) / sizeof(light);
   lights[0].pos.x = 0 * scale;
   lights[0].pos.y = 240 * scale;
   lights[0].pos.z = -100 * scale;
   lights[0].intensity.red = 1;
   lights[0].intensity.green = 1;
   lights[0].intensity.blue = 1;
   
   lights[1].pos.x = 3200 * scale;
   lights[1].pos.y = 3000 * scale;
   lights[1].pos.z = -1000 * scale;
   lights[1].intensity.red = 0.6;
   lights[1].intensity.green = 0.7;
   lights[1].intensity.blue = 1;

   lights[2].pos.x = 600 * scale;
   lights[2].pos.y = 0 * scale;
   lights[2].pos.z = -100 * scale;
   lights[2].intensity.red = 0.3;
   lights[2].intensity.green = 0.5;
   lights[2].intensity.blue = 1;
   
   /* Will contain the raw image */
	/* dynamically allocate framebuffer */
   unsigned char *img;
   img = malloc(sizeof(char) * (3*WIDTH*HEIGHT));

   // Create an array of thread handles
   pthread_t *thread_handles = malloc(sizeof(pthread_t) * threads);

   pthread_mutex_t current_sphere_mutex, rgb_mutex;

   pthread_mutex_init(&current_sphere_mutex, NULL);
   pthread_mutex_init(&rgb_mutex, NULL);
   
   int x, y;

	/*** start timing here ****/

   // we have to use gettimeofday because clock() is not thread safe and will not give correct results
   struct timeval start, end;

   gettimeofday(&start, NULL);

   for(y=0;y<HEIGHT;y++){
      for(x=0;x<WIDTH;x++){
         
         float red = 0;
         float green = 0;
         float blue = 0;
         
         int level = 0;
         float coef = 1.0;
         
         r.start.x = x;
         r.start.y = y;
         r.start.z = -2000;
         
         r.dir.x = 0;
         r.dir.y = 0;
         r.dir.z = 1;
         
         do{
            /* Find closest intersection */
            float t = 20000.0f;
            int currentSphere = -1;

            // Calculate the number of spheres per thread
            int spheresPerThread = num_spheres / threads;
            int extraSpheres = num_spheres % threads;

            // Set a minimum number of spheres per thread
            if (spheresPerThread == 0)
            {
               spheresPerThread = 1; // Minimum one sphere per thread
               extraSpheres = 0;     // No extra spheres in this case
            }

            // Create threads to determine the closest sphere to the ray
            for (int i = 0; i < threads; i++) {
               // In the event that there are more spheres than threads, we don't want to create more threads than there are spheres (in this case there are only 3 spheres but we could have more)
               if (i > num_spheres-1) {
                  break;
               }

               int factor = (num_spheres / threads);
               if (threads > num_spheres || factor == 0) factor = 1;

               SphereThreadData *data = malloc(sizeof(SphereThreadData));
               data->thread_id = i;
               data->start = i * spheresPerThread;

               // If this is the last thread, it will be responsible for the extra spheres
               data->end = (i == (threads - 1)) ? (data->start + spheresPerThread + extraSpheres) : (data->start + spheresPerThread);
               data->r = r;

               // Set the pointers to the t value and the current sphere so that the threads can update them
               data->t = &t;
               data->currentSphere = &currentSphere;

               data->spheres = spheres;
               data->mutex = &current_sphere_mutex;

               int result = pthread_create(&thread_handles[i], NULL, determine_current_sphere, (void *) data);
               if (result != 0) {
                  printf("Error creating thread %d to determine current sphere\n", i);
                  exit(1);
               }
            }

            // Join the threads
            for (int i = 0; i < threads; i++) {
               // In the event that there are more spheres than threads, we don't want to create more threads than there are spheres (in this case there are only 3 spheres but we could have more)
               if (i > num_spheres-1) break;

               int result = pthread_join(thread_handles[i], NULL);
               if (result != 0) {
                  printf("Error joining thread %d to determine current sphere\n", i);
                  exit(1);
               }
            }

            // If there is no intersection, break out of the loop
            if(currentSphere == -1) break;
            
            vector scaled = vectorScale(t, &r.dir);
            vector newStart = vectorAdd(&r.start, &scaled);
            
            /* Find the normal for this new vector at the point of intersection */
            vector n = vectorSub(&newStart, &spheres[currentSphere].pos);
            float temp = vectorDot(&n, &n);
            if(temp == 0) break;
            
            temp = 1.0f / sqrtf(temp);
            n = vectorScale(temp, &n);

            /* Find the material to determine the colour */
            material currentMat = materials[spheres[currentSphere].material];
            
            /* Find the value of the light at this point */

            // Calculate the number of lights per thread
            int lightsPerThread = num_lights / threads;
            int extraLights = num_lights % threads;

            // Set a minimum number of lights per thread
            if (lightsPerThread == 0)
            {
               lightsPerThread = 1; // Minimum one light per thread
               extraLights = 0;     // No extra lights in this case
            }

            // Create threads to calculate the light at this point
            for (int i = 0; i < threads; i++)
            {
               // In the event that there are more lights than threads, we don't want to create more threads than there are lights (in this case there are only 3 lights but we could have more)
               if (i > num_lights-1) break;

               LightThreadData *data = malloc(sizeof(LightThreadData));

               data->thread_id = i;
               data->start = i * lightsPerThread;

               // If this is the last thread, it will be responsible for the extra lights
               data->end = (i == (threads - 1)) ? (data->start + lightsPerThread + extraLights) : (data->start + lightsPerThread);

               // Set the pointers to the rgb values for the current pixel so that the threads can update them
               data->red = &red;
               data->green = &green;
               data->blue = &blue;

               data->coef = coef;

               data->newStart = newStart;
               data->n = n;
               data->currentMat = currentMat;

               data->lights = lights;
               data->spheres = spheres;

               data->mutex = &rgb_mutex;

               int result = pthread_create(&thread_handles[i], NULL, calculate_light, (void *) data);
               if (result != 0) {
                  printf("Error creating thread %d to calculate light\n", i);
                  exit(1);
               }
            }

            // Join the threads
            for (int i = 0; i < threads; i++)
            {
               // In the event that there are more lights than threads, we don't want to create more threads than there are lights (in this case there are only 3 lights but we could have more)
               if (i > num_lights-1) break;

               int result = pthread_join(thread_handles[i], NULL);
               if (result != 0) {
                  printf("Error joining thread %d to calculate light\n", i);
                  exit(1);
               }
            }

            /* Iterate over the reflection */
            coef *= currentMat.reflection;
            
            /* The reflected ray start and direction */
            r.start = newStart;
            float reflect = 2.0f * vectorDot(&r.dir, &n);
            vector tmp = vectorScale(reflect, &n);
            r.dir = vectorSub(&r.dir, &tmp);

            level++;

         }while((coef > 0.0f) && (level < 15));
         
         img[(x + y*WIDTH)*3 + 0] = (unsigned char)min(red*255.0f, 255.0f);
         img[(x + y*WIDTH)*3 + 1] = (unsigned char)min(green*255.0f, 255.0f);
         img[(x + y*WIDTH)*3 + 2] = (unsigned char)min(blue*255.0f, 255.0f);

      }
   }

   gettimeofday(&end, NULL);

   /*** end timing here ***/

   // Calculate the total time taken
   double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

   printf("Time taken: %lf seconds\n", time_taken);
   
	/* only create output file image.ppm when -o is included on command line */
   if (output != 0)
      saveppm("image.ppm", img, WIDTH, HEIGHT);

   // Free the memory allocated for the thread handles
   free(thread_handles);

   // This was not included in the original code but it prevents a memory leak the size of the framebuffer
   free(img); 
   
   return 0;
}
