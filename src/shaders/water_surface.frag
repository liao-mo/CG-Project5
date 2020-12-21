#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;    
    float shininess;
}; 

struct Light {
    //vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform samplerCube skybox;

void main()
{
    vec3 objectColor = vec3(0.5, 0.5, 0.7);

    // ambient
    vec3 ambient = light.ambient * objectColor;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    // vec3 lightDir = normalize(light.position - FragPos);
    vec3 lightDir = normalize(-light.direction);  
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * objectColor;  
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * objectColor;  

    vec3 light_source = ambient + diffuse + specular;

    //skybox reflect and refraction
    float ratio = 1.00 / 1.33;
    vec3 I = normalize(FragPos - viewPos);
    vec3 REF = reflect(I, norm);
    vec3 RFRA = refract(I, norm, ratio);

    vec3 skyReflect = vec3(texture(skybox, REF).rgb);
    vec3 skyRefract = vec3(texture(skybox, RFRA).rgb);
    
    vec3 result = 0.2 * light_source + 0.4 * skyReflect + 0.4 * skyRefract;

    FragColor = vec4(result, 1.0);
} 