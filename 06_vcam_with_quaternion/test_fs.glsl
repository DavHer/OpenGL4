#version 410

in float dist;
out vec4 frag_colour;

void main() {
	frag_colour = vec4 (1.0, 0.0, 0.0, 1.0);
	// Use z position to shader darker to help perception fo distance
	frag_colour.xyz *= dist;
}
