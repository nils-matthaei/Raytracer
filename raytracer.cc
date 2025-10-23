#include "math.h"
#include "geometry.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <sys/types.h>
#include <vector>
#include <fstream>
#include <optional>

// Die folgenden Kommentare beschreiben Datenstrukturen und Funktionen
// Die Datenstrukturen und Funktionen die weiter hinten im Text beschrieben sind,
// hängen höchstens von den vorhergehenden Datenstrukturen ab, aber nicht umgekehrt.
typedef Vector3df Color;
const Color white = {1.0f,1.0f,1.0f};
const Color black = {0.0f,0.0f,0.0f};
const Color red = {1.0f,0.0f,0.0f};
const Color green = {0.0f,1.0f,0.0f};
const Color blue = {0.0f,0.0f,1.0f};

// Ein "Bildschirm", der das Setzen eines Pixels kapselt
// Der Bildschirm hat eine Auflösung (Breite x Höhe)
// Kann zur Ausgabe einer PPM-Datei verwendet werden oder
// mit SDL2 implementiert werden.
class Screen{
  private:
    unsigned int width;
    unsigned int height;
    std::vector<Color> pixels;

  public:
    Screen(const unsigned int width, const unsigned int height)
    : width(width), height(height) {
      for(unsigned int i = 0; i < width * height; ++i){
        this->pixels.push_back({0.0f,0.0f,0.0f});
      }
    }

    unsigned int get_width() {
      return width;
    } 
    
    unsigned int get_height() {
      return height;
    }

    float get_aspect_ratio() {
      return float(width)/height;
    }
  
    void set_pixel(const unsigned int x, const unsigned int y, const Color color){
      const unsigned int index = y * width + x;
      pixels[index] = color;
    }  

    void save_to_file(){
      std::ofstream file;
      file.open("strahl_geht_brrr.ppm");
      if(!file){
        std::cerr << "File not open hahah no business";
        return;  
      }

      file << "P3\n";
      file << width << " " << height << std::endl;
      file << "255";

      for(unsigned int i = 0; i < pixels.size(); ++i){
        if(i % width == 0) file << std::endl;
        Color color = pixels[i];
        unsigned int red_channel = color[0] * 255.999;
        unsigned int green_channel = color[1] * 255.999;
        unsigned int blue_channel = color[2] * 255.999;
        file << red_channel << " " << green_channel << " " << blue_channel << " ";
      }

      file.close();
      std::cout << "Filed the file!";
    }
};


// Eine "Kamera", die von einem Augenpunkt aus in eine Richtung senkrecht auf ein Rechteck (das Bild) zeigt.
// Für das Rechteck muss die Auflösung oder alternativ die Pixelbreite und -höhe bekannt sein.
// Für ein Pixel mit Bildkoordinate kann ein Sehstrahl erzeugt werden.
class Camera{
  private:
    std::unique_ptr<Screen> screen_ptr;
    
    float focal_length = 1.0f;
    float viewport_height = 2.0f;
    float viewport_width;
    Vector3df center = {0.0f,0.0f,0.0f};

    Vector3df viewport_u = {};
    Vector3df viewport_v = {};

    Vector3df pixel_delta_u = {};
    Vector3df pixel_delta_v = {};  

    Vector3df viewport_dist = {0.0f, 0.0f, focal_length};
    Vector3df viewport_upper_left = {};
    Vector3df pixel00_loc = {};

  public:
    Camera(Screen* screen, float _focal_length = 1.0f, float _viewport_height = 2.0f) 
    : screen_ptr(screen), focal_length(_focal_length), viewport_height(_viewport_height) {
      // calculate missing viewport parameters
      viewport_width = viewport_height * screen_ptr->get_aspect_ratio();
      viewport_u = {viewport_width, 0, 0};
      viewport_v = {0, -viewport_height, 0};
      
      // calculate pixel deltas
      pixel_delta_u = (1.0f / screen_ptr->get_width()) * viewport_u;
      pixel_delta_v = (1.0f / screen_ptr->get_height()) * viewport_v;
      
      // calculate location of first pixel
      viewport_upper_left = center - viewport_dist - 0.5f * viewport_u - 0.5f * viewport_v;
      pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
    }

    Ray3df get_ray(unsigned int x, unsigned int y)
    {
      Vector3df pixelxy_loc = pixel00_loc + static_cast<float>(x) * pixel_delta_u + static_cast<float>(y) * pixel_delta_v;
      return {center, pixelxy_loc};
    }
};


// Für die "Farbe" benötigt man nicht unbedingt eine eigene Datenstruktur.
// Sie kann als Vector3df implementiert werden mit Farbanteil von 0 bis 1.
// Vor Setzen eines Pixels auf eine bestimmte Farbe (z.B. 8-Bit-Farbtiefe),
// kann der Farbanteil mit 255 multipliziert  und der Nachkommaanteil verworfen werden.


// Das "Material" der Objektoberfläche mit ambienten, diffusem und reflektiven Farbanteil.
class Material {
  private:
    Color color = {1.0f,1.0f,1.0f};
    bool reflective = false;
    float k_diffuse = 0.9f;
    float k_ambient;

  public:
    Material(Color _color, bool _reflective, float _k_diffuse = 0.9f) 
    : color(_color), reflective(_reflective), k_diffuse(_k_diffuse) {
      k_ambient = 1.0f - k_diffuse;
    }

    Color get_color() {
      return color;
    }

    bool isReflective() {
      return reflective;
    }

    float get_k_diffuse() {
      return k_diffuse;
    }

    float get_k_ambient() {
      return k_ambient;
    }
};


// Ein "Objekt", z.B. eine Kugel oder ein Dreieck, und dem zugehörigen Material der Oberfläche.
// Im Prinzip ein Wrapper-Objekt, das mindestens Material und geometrisches Objekt zusammenfasst.
// Kugel und Dreieck finden Sie in geometry.h/tcc
class Object {
  private:
    Material material;
    Sphere3df sphere;

  public:
    Object();
    
    Object(Material _material, Sphere3df _sphere) 
    : material(_material), sphere(_sphere) {}

    Object(Vector3df center, float radius, Color color, float k_diffuse, bool reflective) 
    :  material(color,reflective,k_diffuse), sphere(center,radius){}

    Material get_material() {
      return material;
    } 

    Sphere3df get_sphere() {
      return sphere;
    }
};

// Define Hitcontext
struct Hitcontext {
  Object object;
  Intersection_Context<float, 3u> intersection_context;
};

// verschiedene Materialdefinition, z.B. Mattes Schwarz, Mattes Rot, Reflektierendes Weiss, ...
// im wesentlichen Variablen, die mit Konstruktoraufrufen initialisiert werden.
const Material matte_white(white, false);
const Material reflective_white(white, true);
const Material matte_black(black, false);

const Material matte_red(red, false);
const Material matte_green(green, false);
const Material matte_blue(blue, false);

// Die folgenden Werte zur konkreten Objekten, Lichtquellen und Funktionen, wie Lambertian-Shading
// oder die Suche nach einem Sehstrahl für das dem Augenpunkt am nächsten liegenden Objekte,
// können auch zusammen in eine Datenstruktur für die gesammte zu
// rendernde "Szene" zusammengefasst werden.

// Die Cornelbox aufgebaut aus den Objekten
// Am besten verwendet man hier einen std::vector< ... > von Objekten.
class Scene {
  private:
    std::vector<Object> objects;

  public:
    Scene(std::vector<Object> _objects) : objects(_objects) {}

    std::optional<Hitcontext> find_nearest_object(Ray3df ray) {
      std::optional<Hitcontext> nearest_context = std::nullopt;
      float nearest_distance = MAXFLOAT; 

      for(Object object : objects) {
        Intersection_Context<float, 3> context;
        if(object.get_sphere().intersects(ray,context)) {
          if(context.t < nearest_distance) {
            nearest_context = {object, context};
            nearest_distance = context.t;
          }
        }
      }
      return nearest_context;
    }
};

// Cornell Box
const float big_offset = 6376.0f;
const Object big_white_ball(matte_white,   {{0.0f, 0.0f, -big_offset - 5.0f},  6371.0f});
const Object big_white_ball_b(matte_white, {{0.0f, -big_offset, 0.0f},  6371.0f});
const Object big_white_ball_t(matte_white, {{0.0f,  big_offset, 0.0f},  6371.0f});
const Object big_blue_ball(matte_blue,     {{-big_offset, 0.0f, -0.0f}, 6371.0f});
const Object big_red_ball(matte_red,       {{ big_offset, 0.0f, -0.0f}, 6371.0f});

// Them balls
const Object green_ball(matte_green, {{3.0f,-4.0f,-8.0f}, 1.0f});
const Object reflective_ball(reflective_white, {{-1.0f,-3.7f,-7.0f}, 1.3f});
// Whole scene
Scene scene({big_white_ball, big_white_ball_b, big_white_ball_t, big_blue_ball, big_red_ball, // Cornell Box 
            green_ball, reflective_ball}); // Balls

// Punktförmige "Lichtquellen" können einfach als Vector3df implementiert werden mit weisser Farbe,
// bei farbigen Lichtquellen müssen die entsprechenden Daten in Objekt zusammengefaßt werden
// Bei mehreren Lichtquellen können diese in einen std::vector gespeichert werden.
typedef Vector3df Lightsource;
const Lightsource lightsource_1= {{3.0f,4.5f,-7.0f}}; 
const Lightsource lightsource_2= {{-3.0f,4.5f,-7.0f}}; 
const std::vector<Lightsource> lightsources = {lightsource_1, lightsource_2};

std::vector<Lightsource> find_light_sources(Intersection_Context<float, 3u> &context){
  std::vector<Lightsource> visible_lights = {};
  // slightly move intersection point out to reduce shadow acne
  const Vector3df intersection_point = context.intersection + 6e-2f * context.normal;  
  
  for(const Lightsource &light : lightsources){
    Vector3df light_direction = light - intersection_point;
    Ray3df shadow_ray(intersection_point,light_direction);
    std::optional<Hitcontext> shadow_context_opt = scene.find_nearest_object(shadow_ray);
    if(!shadow_context_opt) {
      visible_lights.push_back(light);
      continue;
    }
    Intersection_Context<float, 3u> intersection_context = shadow_context_opt->intersection_context;
    if(intersection_context.t > 1.0f){
     visible_lights.push_back(light);  
    }
  }
  return visible_lights;
}

// Sie benötigen eine Implementierung von Lambertian-Shading, z.B. als Funktion
// Benötigte Werte können als Parameter übergeben werden, oder wenn diese Funktion eine Objektmethode eines
// Szene-Objekts ist, dann kann auf die Werte teilweise direkt zugegriffen werden.
// Bei mehreren Lichtquellen muss der resultierende diffuse Farbanteil durch die Anzahl Lichtquellen geteilt werden.

const Color lambertian(Hitcontext hitcontext) {
  Object object = hitcontext.object;
  Intersection_Context<float, 3u> intersection_context = hitcontext.intersection_context;
  std::vector<Lightsource> visible_lights = find_light_sources(intersection_context);

  float shading_factor = object.get_material().get_k_ambient();
  float diffuse_factor = 0.0f;
  for(const Lightsource light : visible_lights){
    Vector3df lightsource_normal = light - intersection_context.intersection;
    lightsource_normal.normalize();
    diffuse_factor += std::max(0.0f,intersection_context.normal * lightsource_normal);
  }
  diffuse_factor *= 1.0f/lightsources.size() * object.get_material().get_k_diffuse();
  shading_factor += diffuse_factor;

  return shading_factor * object.get_material().get_color();
}
// Für einen Sehstrahl aus allen Objekte, dasjenige finden, das dem Augenpunkt am nächsten liegt.
// Am besten einen Zeiger auf das Objekt zurückgeben. Wenn dieser nullptr ist, dann gibt es kein sichtbares Objekt.

// Die rekursive raytracing-Methode. Am besten ab einer bestimmten Rekursionstiefe (z.B. als Parameter übergeben) abbrechen.
Color tracin_them_rays(Ray3df ray, unsigned int max_depth) {
  Color color = black;
  if(max_depth > 0){
    std::optional<Hitcontext> context_opt = scene.find_nearest_object(ray);
    if(!context_opt) {return black;}
    // Recursion if reflective object
    if(context_opt->object.get_material().isReflective()) {
      Intersection_Context<float, 3u> context = context_opt->intersection_context;
      Vector3df normal = context.normal;
      Vector3df intersection = context.intersection + 6e-2f*normal;
      Ray3df reflective = {intersection, ray.direction.get_reflective(normal)};
      return 0.9f * tracin_them_rays(reflective, max_depth - 1) + 0.1f*white;
    }
    // Lambertian shading
    color = lambertian( *context_opt );
  }
  return color;
}

int main(void) {
  // Bildschirm erstellen
  // Kamera erstellen
  // Für jede Pixelkoordinate x,y
  //   Sehstrahl für x,y mit Kamera erzeugen
  //   Farbe mit raytracing-Methode bestimmen
  //   Beim Bildschirm die Farbe für Pixel x,y, setzten
  Screen* screen = new Screen(800,600);
  Camera* camera = new Camera(screen, 1.5, 3.0);

  Color color = {};
  for(unsigned int x = 0; x < screen->get_width(); ++x) {
    for(unsigned int y = 0; y < screen->get_height(); ++y) {
      Ray3df ray = camera->get_ray(x,y);
      color = tracin_them_rays(ray, 5);
      screen->set_pixel(x,y,color);
    }
  }

  screen->save_to_file();
  return 0;
}

