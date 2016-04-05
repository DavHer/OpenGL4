/*
 * obj_parser.h
 *
 *  Created on: Apr 4, 2016
 *      Author: david
 */

#ifndef OBJ_PARSER_H_
#define OBJ_PARSER_H_

#include <stdbool.h>

bool load_obj_file(const char* file_name,
				   float* &points,
				   float* &tex_coords,
				   float* &normals,
				   int& point_count);



#endif /* OBJ_PARSER_H_ */
