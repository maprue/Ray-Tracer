#include <cstdio>
#include <cstdlib>
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>

#include "raytracer.hpp"
#include "image.hpp"


void Raytracer::render(const char *filename, const char *depth_filename,
                       Scene const &scene)
{
    // Alloue les deux images qui seront sauvegardées à la fin du programme.
    Image colorImage(scene.resolution[0], scene.resolution[1]);
    Image depthImage(scene.resolution[0], scene.resolution[1]);
    
    // Crée le zBuffer.
    double *zBuffer = new double[scene.resolution[0] * scene.resolution[1]];
    for(int i = 0; i < scene.resolution[0] * scene.resolution[1]; i++) {
        zBuffer[i] = DBL_MAX;
    }

	// @@@@@@ VOTRE CODE ICI
	// Calculez les paramètres de la caméra pour les rayons. Référez-vous aux slides pour les détails.
	//!!! NOTE UTILE : tan() prend des radians plutot que des degrés. Utilisez deg2rad() pour la conversion.
	//!!! NOTE UTILE : Le plan de vue peut être n'importe où, mais il sera implémenté différement.
	// Vous trouverez des références dans le cours.

	Vector upNormalized = scene.camera.up.normalized();
	Vector lookAt = scene.camera.center - scene.camera.position;
	double distance = lookAt.length();
	lookAt = lookAt / distance;  //Pour qu'il soit unitaire
	double halfScreenHeight = tan(deg2rad(scene.camera.fovy / 2)) * distance;
	double screenHeight = halfScreenHeight * 2;
	double screenWidth = scene.camera.aspect * screenHeight;

	int nx = scene.resolution[0];
	int ny = scene.resolution[1];

	double pixelHeight = screenHeight / ny;  //deltaV
	double pixelWidth = screenWidth / nx;   // deltaU

	Vector u = -upNormalized.cross(lookAt).normalized(); // À droite de la caméra
	Vector bigO = scene.camera.position + lookAt * distance + u * (-screenWidth / 2) + -upNormalized * halfScreenHeight;//Coin de l'écran à partir duquel je vais balayer

    // Itère sur tous les pixels de l'image.
    for(int y = 0; y < scene.resolution[1]; y++) {
        for(int x = 0; x < scene.resolution[0]; x++) {

            // Génère le rayon approprié pour ce pixel.
			Ray ray;
			if (scene.objects.empty())
			{
				// Pas d'objet dans la scène --> on rend la scène par défaut.
				// Pour celle-ci, le plan de vue est à z = 640 avec une largeur et une hauteur toute deux à 640 pixels.
				ray = Ray(scene.camera.position, (Vector(-320, -320, 640) + Vector(x + 0.5, y + 0.5, 0) - scene.camera.position).normalized());
			}
			else
			{
				// @@@@@@ VOTRE CODE ICI
				// Mettez en place le rayon primaire en utilisant les paramètres de la caméra.
				//!!! NOTE UTILE : tous les rayons dont les coordonnées sont exprimées dans le
				//                 repère monde doivent avoir une direction normalisée.
				ray = Ray(scene.camera.position, (bigO + (x + 0.5) * pixelWidth * u + (y + 0.5) * pixelHeight * upNormalized - scene.camera.position).normalized());
				
			}

            // Initialise la profondeur de récursivité du rayon.
            int rayDepth = 0;
           
            // Notre lancer de rayons récursif calculera la couleur et la z-profondeur.
            Vector color;

            // Ceci devrait être la profondeur maximum, correspondant à l'arrière plan.
            // NOTE : Ceci suppose que la direction du rayon est de longueur unitaire (normalisée)
			//        et que l'origine du rayon est à la position de la caméra.
            double depth = scene.camera.zFar;

            // Calcule la valeur du pixel en lançant le rayon dans la scène.
            trace(ray, rayDepth, scene, color, depth);

            // Test de profondeur
            if(depth >= scene.camera.zNear && depth <= scene.camera.zFar && 
                depth < zBuffer[x + y*scene.resolution[0]]) {
                zBuffer[x + y*scene.resolution[0]] = depth;

                // Met à jour la couleur de l'image (et sa profondeur)
                colorImage.setPixel(x, y, color);
                depthImage.setPixel(x, y, (depth-scene.camera.zNear) / 
                                        (scene.camera.zFar-scene.camera.zNear));
            }
        }

		// Affiche les informations de l'étape
		if (y % 100 == 0)
		{
			printf("Row %d pixels finished.\n", y);
		}
    }

	// Sauvegarde l'image
    colorImage.writeBMP(filename);
    depthImage.writeBMP(depth_filename);

	printf("Ray tracing finished with images saved.\n");

    delete[] zBuffer;
}


bool Raytracer::trace(Ray const &ray, 
                 int &rayDepth,
                 Scene const &scene,
                 Vector &outColor, double &depth)
{    
	// Incrémente la profondeur du rayon.
    rayDepth++;

	// Initialise hit
	Intersection hit;
	hit.depth = depth;

	bool boolHit = false;

    // - itérer sur tous les objets en appelant   Object::intersect.
    // - ne pas accepter les intersections plus lointaines que la profondeur donnée.
    // - appeler Raytracer::shade avec l'intersection la plus proche.
    // - renvoyer true ssi le rayon intersecte un objet.
	if (scene.objects.empty())
	{
		// Pas d'objet dans la scène --> on rend la scène par défaut :
		// Par défaut, un cube est centré en (0, 0, 1280 + 160) avec une longueur de côté de 320, juste en face de la caméra.
		// Test d'intersection :
		double x = 1280 / ray.direction[2] * ray.direction[0] + ray.origin[0];
		double y = 1280 / ray.direction[2] * ray.direction[1] + ray.origin[1];
		if ((x <= 160) && (x >= -160) && (y <= 160) && (y >= -160))
		{
			// S'il y a intersection :
			Material m; m.emission = Vector(16.0, 0, 0); m.reflect = 0; // seulement pour le matériau par défaut ; vous devrez utiliser le matériau de l'objet intersecté
			outColor = shade(ray, rayDepth, hit, m, scene);
			depth = 1280;	// la profondeur devrait être mise à jour dans la méthode Object::intersect()
		}
	}
	else
	{
		// - itérer sur tous les objets en appelant   Object::intersect.
		// - ne pas accepter les intersections plus lointaines que la profondeur donnée.
		// - appeler Raytracer::shade avec l'intersection la plus proche.
		// - renvoyer true ssi le rayon intersecte un objet.
		// @@@@@@ VOTRE CODE ICI
		// Si intersection return true
		// Notez que pour Object::intersect(), le paramètre hit correspond à celui courant.
		// Votre intersect() devrait être implémenté pour exclure toute intersection plus lointaine que hit.depth
		
		for (auto objectIter = scene.objects.begin(); objectIter != scene.objects.end(); objectIter++) // Go through all objects
		{	// S'il y a intersection :
			bool pass = (**objectIter).intersect(ray, hit);

			if ( pass ) {
				outColor = shade(ray, rayDepth, hit, (**objectIter).material, scene);	
				if (hit.depth < depth)
				{
					depth = hit.depth;
				}

				boolHit = true;
			}
		}
	}
   
	// Décrémente la profondeur du rayon.
    rayDepth--;

    return boolHit;
}


//Fonction de : https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
void fresnel(const Vector& I, const Vector& N, const double& ior, double& kr)
{
	double cosi = std::clamp(-1.0, 1.0, I.dot(N));
	double etai = 1, etat = ior;
	if (cosi > 0) { std::swap(etai, etat); }
	// Compute sini using Snell's law
	double sint = etai / etat * std::sqrt(std::max(0.0, 1 - cosi * cosi));
	// Total internal reflection
	if (sint >= 1) {
		kr = 1;
	}
	else {
		double cost = std::sqrt(std::max(0.0, 1 - sint * sint));
		cosi = std::abs(cosi);
		double Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
		double Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
		kr = (Rs * Rs + Rp * Rp) / 2;
	}
}

//Fonction de : https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
Vector refract(const Vector& I, const Vector& N, const double& ior)
{
	double cosi = std::clamp(-1.0, 1.0, I.dot(N));
	double etai = 1, etat = ior;
	Vector n = N;
	if (cosi < 0) { cosi = -cosi; }
	else { std::swap(etai, etat); n = -N; }
	double eta = etai / etat;
	double k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? 0 : eta * I + (eta * cosi - std::sqrt(k)) * n;
}


Vector Raytracer::shade(Ray const &ray,
                 int &rayDepth,
                 Intersection const &intersection,
                 Material const &material,
                 Scene const &scene)
{
    // - itérer sur toutes les sources de lumières, calculant les contributions ambiant/diffuse/speculaire
    // - utiliser les rayons d'ombre pour déterminer les ombres
    // - intégrer la contribution de chaque lumière
    // - inclure l'émission du matériau de la surface, s'il y a lieu
    // - appeler Raytracer::trace pour les couleurs de reflection/refraction
    // Ne pas réfléchir/réfracter si la profondeur de récursion maximum du rayon a été atteinte !
	//!!! NOTE UTILE : facteur d'atténuation = 1.0 / (a0 + a1 * d + a2 * d * d)..., la lumière ambiante ne s'atténue pas, ni n'est affectée par les ombres
	//!!! NOTE UTILE : n'acceptez pas les intersection des rayons d'ombre qui sont plus loin que la position de la lumière
	//!!! NOTE UTILE : pour chaque type de rayon, i.e. rayon d'ombre, rayon reflechi, et rayon primaire, les profondeurs maximales sont différentes
	Vector diffuse(0);
	Vector ambient(0);
	Vector specular(0);

	Vector rayDir = ray.direction.normalized();

	for (auto lightIter = scene.lights.begin(); lightIter != scene.lights.end(); lightIter++)
	{
		// @@@@@@ VOTRE CODE ICI
		// Calculez l'illumination locale ici, souvenez-vous d'ajouter les lumières ensemble.
		// Testez également les ombres ici, si un point est dans l'ombre aucune couleur (sauf le terme ambient) ne devrait être émise.
		double a0 = (*lightIter).attenuation[0];
		double a1 = (*lightIter).attenuation[1];
		double a2 = (*lightIter).attenuation[2];

		double d = ((*lightIter).position - intersection.position).length(); 

		double attenuation = 1.0 / (a0 + a1 * d + a2 * d * d);
		Vector E = (ray.origin - intersection.position).normalized();
		Vector N = intersection.normal;
		Vector L = ((*lightIter).position - intersection.position).normalized(); // light direction normalized result of (vertexPos - lightPos).
		Vector H = ((L + E) / 2.0).normalized();
		Vector  R = ((-L).reflect(N)).normalized();

		ambient += material.ambient * (*lightIter).ambient;

		// Calc shadow
		Ray shadowRay(intersection.position+L*(2e-6), L);
		Vector shadowOut;

		double depth = ((*lightIter).position - shadowRay.origin).length();

		bool shadow = trace(shadowRay, rayDepth, scene, shadowOut, depth);

		if (!shadow)
		{
			diffuse += attenuation*(material.diffuse * (*lightIter).diffuse * std::max(N.dot(L), 0.0));
			specular += attenuation*(material.specular * (*lightIter).specular * std::pow(std::max(R.dot(E),0.0), material.shininess)); 
		}		
	}

	Vector reflectedLight(0);
	Vector refractedLight(0);	
	double kr = 1;
	if ((!(ABS_FLOAT(material.reflect) < 1e-6)) && (rayDepth < MAX_RAY_RECURSION))
		{		
		// @@@@@@ VOTRE CODE ICI
		// Calculez la couleur réfléchie en utilisant trace() de manière récursive.

		if (material.reflect > 0.0) { // object is shiny!
			Vector R = (rayDir.reflect(intersection.normal)).normalized();
			Ray reflectionRay(intersection.position + R * (2e-6), R);

			double depth = DBL_MAX;

			bool reflection = trace(reflectionRay, rayDepth, scene, reflectedLight, depth);	
		}
		
		if (material.transparent > 0.0) { //object is transparent!
	
			// Formules refraction : https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel
			double epsilon = 1e-6; //so not directly on intersection
			
			fresnel(rayDir, intersection.normal, material.refract, kr);
			
			bool outside = rayDir.dot(intersection.normal) < 0;
			Vector bias = epsilon * intersection.normal;
			
			if (kr < 1) { // If not total internal reflection
				Vector refractionDirection = (refract(rayDir, intersection.normal, material.refract).normalized());
				Vector refractionRayOrig = outside ? intersection.position - bias : intersection.position + bias;
				Ray refractionRay(refractionRayOrig, refractionDirection);
				double depth = DBL_MAX;
				bool refraction = trace(refractionRay, rayDepth, scene, refractedLight, depth);
			}
		}
	}
	return material.emission + ambient + diffuse + specular + kr * material.reflect * reflectedLight + refractedLight * (1 - kr) * material.refract;
}