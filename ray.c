
/* The original ray tracing code was taken from the web site:
    https://www.purplealienplanet.com/node/23 */

/* A simple ray tracer */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> /* Needed for boolean datatype */
#include <math.h>
#include <string.h>

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
   
   int x, y;

	/*** start timing here ****/

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
            
            unsigned int i;
            for(i = 0; i < 3; i++){
               if(intersectRaySphere(&r, &spheres[i], &t))
                  currentSphere = i;
            }
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
            unsigned int j;
            for(j=0; j < 3; j++){
               light currentLight = lights[j];
               vector dist = vectorSub(&currentLight.pos, &newStart);
               if(vectorDot(&n, &dist) <= 0.0f) continue;
               float t = sqrtf(vectorDot(&dist,&dist));
               if(t <= 0.0f) continue;
               
               ray lightRay;
               lightRay.start = newStart;
               lightRay.dir = vectorScale((1/t), &dist);
               
               /* Calculate shadows */
               bool inShadow = false;
               unsigned int k;
               for (k = 0; k < 3; ++k) {
                  if (intersectRaySphere(&lightRay, &spheres[k], &t)){
                     inShadow = true;
                     break;
                  }
               }
               if (!inShadow){
                  /* Lambert diffusion */
                  float lambert = vectorDot(&lightRay.dir, &n) * coef; 
                  red += lambert * currentLight.intensity.red * currentMat.diffuse.red;
                  green += lambert * currentLight.intensity.green * currentMat.diffuse.green;
                  blue += lambert * currentLight.intensity.blue * currentMat.diffuse.blue;
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

      /*** end timing here ***/
   
	/* only create output file image.ppm when -o is included on command line */
   if (output != 0)
      saveppm("image.ppm", img, WIDTH, HEIGHT);

   // This was not included in the original code but it prevents a memory leak the size of the framebuffer
   free(img); 
   
   return 0;
}
