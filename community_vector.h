/* Copyright (c) 2026, Frederic Descamps.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#define LOG_COMPONENT_TAG "community_vector"


#include <mysql/components/component_implementation.h>
#include <mysql/components/services/log_builtins.h> /* LogComponentErr */
#include <mysqld_error.h>                           /* Errors */
#include <mysql/components/services/udf_metadata.h>
#include <mysql/components/services/udf_registration.h>
#include <mysql/components/services/mysql_runtime_error_service.h>
#include <mysql/components/services/mysql_string.h>

#include <list>
#include <string>
#include <vector>
#include <sstream>

#include <cmath>
#include <cstring>
#include <new>

#include "vector-common/vector_constants.h"

// Size of the per-UDF result buffer allocated in the *_udf_init functions
static constexpr size_t max_vector_bytes =
    vector_constants::max_dimensions * sizeof(float);

// Copies the result floats into the caller's buffer (initid->ptr).
static char *store_vector(const std::vector<float> &result, char *out,
                          unsigned long *length) {
  if (result.size() > vector_constants::max_dimensions) {
    mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                                    ER_UDF_ERROR, 0, "vector operation",
                                    "Data out of range");
    return nullptr;
  }
  *length = result.size() * sizeof(float);
  memcpy(out, result.data(), *length);
  return out;
}


void populate_vector(uint32_t vec_dim, float *vec1, 
		       std::vector<float> &vector1) {
    vector1.clear();
    vector1.reserve(vec_dim);

    for (uint32_t i = 0; i < vec_dim; i++) {
        float value1;
        memcpy(&value1, vec1 + i, sizeof(float));
        vector1.push_back(value1);
    }
}

std::string std_vector_to_string(const std::vector<float>& vec) {
    std::ostringstream oss;
    oss << "[";

    for (size_t i = 0; i < vec.size(); ++i) {
        // Set precision and scientific notation
        oss << std::scientific << vec[i];

        // Add a comma if it's not the last element
        if (i != vec.size()-1) {
            oss << ",";
        }
    }

    oss << "]";
    return oss.str();
}

void vector_to_binary(const std::vector<float> &vector1, float *&vec1) {
    vec1 = new float[vector1.size()];

    memcpy(vec1, vector1.data(), vector1.size() * sizeof(float));
}

static float calculate_euclidian_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float sum = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
	   double diff = vector1[i] - vector2[i];
	   sum += diff * diff;
   }

   return sqrtf(sum);
}

static float calculate_dotproduct_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float sum = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
	   sum += vector1[i] * vector2[i];
   }

   return sum;
}

static float calculate_cosine_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float sum = 0.0;
   float magnitude_vec1 = 0.0;
   float magnitude_vec2 = 0.0;

   for (size_t i = 0; i < vector1.size(); ++i) {
	   sum += vector1[i] * vector2[i];
	   magnitude_vec1 += vector1[i] * vector1[i];
	   magnitude_vec2 += vector2[i] * vector2[i];
   }

   if (magnitude_vec1 == 0.0 || magnitude_vec2 == 0.0) {
	   // Error with zero vector
	   mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
           ER_UDF_ERROR, 0, "vector_cosine_distance", "Vectors must not be zero vectors.");
   	   return 0;
   }

   double cosine_similarity = sum / (sqrt(magnitude_vec1) * sqrt(magnitude_vec2));
   return 1.0 - cosine_similarity;
}

static float calculate_manhattan_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;


   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float sum = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
           sum += fabs(vector1[i] - vector2[i]);
   }

   return sum;
}

static float calculate_chebyshev_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float maxdiff = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
	   float diff = fabs(vector1[i] - vector2[i]);
	   if (diff > maxdiff) {
		   maxdiff = diff;
           }
   }

   return maxdiff;
}

static float calculate_jaccard_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   int intersection = 0;
   int unionset = 0;

   for (size_t i = 0; i < vector1.size(); ++i) {
	   if(vector1[i] || vector2[i]) {
		   ++unionset;
	   }
	   if(vector1[i] && vector2[i]) {
		   ++intersection;
	   }
   }

   if (unionset == 0) return 0.0;

   return 1.0 - static_cast<float>(intersection) / unionset;
}

static int calculate_hamming_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   int distance = 0;

   for (size_t i = 0; i < vector1.size(); ++i) {
           if(vector1[i] != vector2[i]) {
                   ++distance;
           }
   }

   return distance;
}

static float calculate_bhattacharyya_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float sum = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
           sum += sqrt(vector1[i] * vector2[i]);
   }

   return -log(sum);
}

static float calculate_canberra_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float sum = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
	   float denominator = fabs(vector1[i]) + fabs(vector2[i]);
	   if (denominator != 0) {
		   sum += fabs(vector1[i] - vector2[i]) / denominator;
	   }
   }

   return sum;
}

static float calculate_hellinger_distance(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float sum = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
           float term = sqrt(vector1[i]) - sqrt(vector2[i]);
	   sum += term * term;
   }

   return sqrt(sum) / sqrt(2);
}

static float calculate_minkowski_distance(uint32_t vec_dim, float *vec1, float *vec2, float p) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float sum = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
	   sum += pow(fabs(vector1[i] - vector2[i]), p);
   }

   return pow(sum, 1.0 / p);
}

static float calculate_kullback_leibler_divergence(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   float divergence = 0.0;
   for (size_t i = 0; i < vector1.size(); ++i) {
	   if(vector1[i] > 0 && vector2[i] > 0 ) {
		   divergence += vector1[i] * log(vector1[i] / vector2[i]);
	   }
   }

   return divergence;
}

static float calculate_jensen_shannon_divergence(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   std::vector<float> mid_point(vector1.size());
   for (size_t i = 0; i < vector1.size(); ++i) {
   	mid_point[i] = 0.5 * (vector1[i] + vector2[i]);
   }

   auto kl_div = [](const std::vector<float>& p, const std::vector<float>& q) {
        float divergence = 0.0;
        for (size_t i = 0; i < p.size(); ++i) {
            if (p[i] > 0 && q[i] > 0) {
                divergence += p[i] * log(p[i] / q[i]);
            }
        }
        return divergence;
    };

    float js_div = 0.5 * (kl_div(vector1, mid_point) + kl_div(vector2, mid_point));

    return js_div;
}

static char* vector_addition(uint32_t vec_dim, float *vec1, float *vec2, char *out, unsigned long *length) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   std::vector<float> result(vector1.size());
   for (size_t i = 0; i < vector1.size(); ++i) {
           result[i] = vector1[i] + vector2[i];
   }
   
   return store_vector(result, out, length);
}

static char* vector_subtraction(uint32_t vec_dim, float *vec1, float *vec2, char *out, unsigned long *length) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   std::vector<float> result(vector1.size());
   for (size_t i = 0; i < vector1.size(); ++i) {
           result[i] = vector1[i] - vector2[i];
   }

   return store_vector(result, out, length);
}

static char* vector_scalar_multiplication(uint32_t vec_dim, float *vec1, float p, char *out, unsigned long *length) {

   std::vector<float> vector1;

   populate_vector(vec_dim, vec1,vector1);

   std::vector<float> result(vector1.size());
   for (size_t i = 0; i < vector1.size(); ++i) {
           result[i] = vector1[i] * p;
   }

   return store_vector(result, out, length);

}

static char* vector_dot_product(uint32_t vec_dim, float *vec1, float *vec2, char *out, unsigned long *length) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   std::vector<float> result(vector1.size());
   for (size_t i = 0; i < vector1.size(); ++i) {
           result[i] = vector1[i] * vector2[i];
   }

   return store_vector(result, out, length);
}

static char* vector_cross_product(uint32_t vec_dim, float *vec1, float *vec2, char *out, unsigned long *length) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   std::vector<float> result(3);
   result[0] =  vector1[1] * vector2[2] - vector1[2] * vector2[1];
   result[1] =  vector1[2] * vector2[0] - vector1[0] * vector2[2];
   result[2] =  vector1[0] * vector2[1] - vector1[1] * vector2[0];

   return store_vector(result, out, length);
}

static float vector_norm(uint32_t vec_dim, float *vec1) {

   std::vector<float> vector1;
   float sum = 0.0;

   populate_vector(vec_dim, vec1,vector1);
   
   for (const auto& value : vector1) {
	   sum += value * value;
   }

   return sqrt(sum);
}

double dot_product(const std::vector<float>& vec1, const std::vector<float>& vec2) {

    double result = 0.0;
    for (size_t i = 0; i < vec1.size(); ++i) {
        result += vec1[i] * vec2[i];
    }
    return result;
}

static float vector_angle(uint32_t vec_dim, float *vec1, float *vec2) {

   std::vector<float> vector1;
   std::vector<float> vector2;
 
   populate_vector(vec_dim, vec1,vector1);
   populate_vector(vec_dim, vec2, vector2);

   double dot_prod = dot_product(vector1, vector2);
   double norm1 = vector_norm(vec_dim, vec1);
   double norm2 = vector_norm(vec_dim, vec2);

   return acos(dot_prod / (norm1 * norm2));
}

static char* vector_normalize(uint32_t vec_dim, float *vec1, char *out, unsigned long *length) {

   std::vector<float> vector1;
   
   float norm = vector_norm(vec_dim, vec1);

   if (norm == 0.0) {
   	mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
        ER_UDF_ERROR, 0, "vector_normalize", "Cannot normalize a zero vector.");
        return 0;
   }
   populate_vector(vec_dim, vec1, vector1);

   std::vector<float> result(vector1.size());
   for (size_t i = 0; i < vector1.size(); ++i) {
           result[i] = vector1[i] /norm;
   }

   return store_vector(result, out, length);
}

static char* vector_projection(uint32_t vec_dim, float *vec1, float *vec2, char *out, unsigned long *length) {

   std::vector<float> vector1;
   std::vector<float> vector2;

   populate_vector(vec_dim, vec1, vector1);
   populate_vector(vec_dim, vec2, vector2);

   double dot_prod = dot_product(vector1, vector2);
   double norm2 = vector_norm(vec_dim, vec2);
   float scale = dot_prod / (norm2 * norm2);

   return vector_scalar_multiplication(vec_dim, vec2, scale, out, length);
}

extern REQUIRES_SERVICE_PLACEHOLDER(log_builtins);
extern REQUIRES_SERVICE_PLACEHOLDER(log_builtins_string);
extern REQUIRES_SERVICE_PLACEHOLDER(udf_registration);
extern REQUIRES_SERVICE_PLACEHOLDER(mysql_udf_metadata);
extern REQUIRES_SERVICE_PLACEHOLDER(mysql_runtime_error);

extern SERVICE_TYPE(log_builtins) * log_bi;
extern SERVICE_TYPE(log_builtins_string) * log_bs;
