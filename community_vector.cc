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

#define NO_SIGNATURE_CHANGE 0
#define SIGNATURE_CHANGE 1

#include "community_vector.h"

REQUIRES_SERVICE_PLACEHOLDER(log_builtins);
REQUIRES_SERVICE_PLACEHOLDER(log_builtins_string);
REQUIRES_SERVICE_PLACEHOLDER(udf_registration);
REQUIRES_SERVICE_PLACEHOLDER(mysql_udf_metadata);
REQUIRES_SERVICE_PLACEHOLDER(mysql_runtime_error);

SERVICE_TYPE(log_builtins) * log_bi;
SERVICE_TYPE(log_builtins_string) * log_bs;

class udf_list {
  typedef std::list<std::string> udf_list_t;

 public:
  ~udf_list() { unregister(); }
  bool add_scalar(const char *func_name, enum Item_result return_type,
                  Udf_func_any func, Udf_func_init init_func = NULL,
                  Udf_func_deinit deinit_func = NULL) {
    if (!mysql_service_udf_registration->udf_register(
            func_name, return_type, func, init_func, deinit_func)) {
      set.push_back(func_name);
      return false;
    }
    return true;
  }

  bool unregister() {
    udf_list_t delete_set;
    /* try to unregister all of the udfs */
    for (auto udf : set) {
      int was_present = 0;
      if (!mysql_service_udf_registration->udf_unregister(udf.c_str(),
                                                          &was_present) ||
          !was_present)
        delete_set.push_back(udf);
    }

    /* remove the unregistered ones from the list */
    for (auto udf : delete_set) set.remove(udf);

    /* success: empty set */
    if (set.empty()) return false;

    /* failure: entries still in the set */
    return true;
  }

 private:
  udf_list_t set;
} * list;

namespace udf_impl {

void error_msg_size() {	
	mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector operation",
                "both vectors must to have the same size");
}

// UDF to calculate the Euclidian distance between two vectors

static bool vector_euclidian_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_euclidian_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_euclidian_distance_udf_deinit(UDF_INIT *) {}

double vector_euclidian_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_euclidian_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the dot product of two vectors

static bool vector_dotproduct_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_dotproduct_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_dotproduct_distance_udf_deinit(UDF_INIT *) {}

double vector_dotproduct_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_dotproduct_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the cosine distance between two vectors

static bool vector_cosine_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_cosine_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_cosine_distance_udf_deinit(UDF_INIT *) {}

double vector_cosine_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_cosine_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the Manhattan distance between two vectors (L1 Norm)

static bool vector_manhattan_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_manhattan_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_manhattan_distance_udf_deinit(UDF_INIT *) {}

double vector_manhattan_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_manhattan_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the Chebyshev distance between two vectors (L∞ Norm)

static bool vector_chebyshev_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_chebyshev_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_chebyshev_distance_udf_deinit(UDF_INIT *) {}

double vector_chebyshev_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_chebyshev_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the Jaccard distance between two vectors

static bool vector_jaccard_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_jaccard_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_jaccard_distance_udf_deinit(UDF_INIT *) {}

double vector_jaccard_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_jaccard_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the Hamming distance between two vectors

static bool vector_hamming_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_hamming_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_hamming_distance_udf_deinit(UDF_INIT *) {}

int vector_hamming_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    int distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_hamming_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the Bhattacharyya distance between two vectors

static bool vector_bhattacharyya_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_bhattacharyya_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_bhattacharyya_distance_udf_deinit(UDF_INIT *) {}

double vector_bhattacharyya_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_bhattacharyya_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the Canberra distance between two vectors

static bool vector_canberra_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_canberra_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_canberra_distance_udf_deinit(UDF_INIT *) {}

double vector_canberra_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_canberra_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the Hellinger distance between two vectors

static bool vector_hellinger_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_hellinger_distance",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_hellinger_distance_udf_deinit(UDF_INIT *) {}

double vector_hellinger_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_hellinger_distance(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to calculate the Minkowski distance between two vectors

static bool vector_minkowski_distance_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 3) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_minkowski_distance",
                "this function requires 3 parameters");
      return true;
  }
  if (args->arg_type[2] != REAL_RESULT && args->arg_type[2] != DECIMAL_RESULT) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_minkowski_distance",
		"the third parameteter must be a float or a decimal (example: 3.0e-0 or 3.0)");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_minkowski_distance_udf_deinit(UDF_INIT *) {}

double vector_minkowski_distance_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }
    
    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    double p = 0.0;
    if (args->arg_type[2] == DECIMAL_RESULT) {
	    p = atof(args->args[2]); 
    } else {
            p = *((double *)args->args[2]);
    }

    *outp = 0;

    distance = calculate_minkowski_distance(dim_vec1, vec1, vec2, p);

    return distance;
}

// UDF to measure how one probability distribution diverge from a second, expected probability distribution: Kullback-Leibler divergence

static bool vector_kullback_leibler_divergence_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_kullback_leibler_divergence",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_kullback_leibler_divergence_udf_deinit(UDF_INIT *) {}

double vector_kullback_leibler_divergence_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_kullback_leibler_divergence(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to implement a symmetric version of the Kullback-Leibler Divergence that measures the similarity between two probability distributions: Jensen-Shannon Divergence

static bool vector_jensen_shannon_divergence_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_jensen_shannon_divergence",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_jensen_shannon_divergence_udf_deinit(UDF_INIT *) {}

double vector_jensen_shannon_divergence_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float distance = 0;
    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    *outp = 0;

    distance = calculate_jensen_shannon_divergence(dim_vec1, vec1, vec2);

    return distance;
}

// UDF to implement a vector addition function between two vectors

static bool vector_addition_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_addition",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  initid->max_length = max_vector_bytes;
  initid->ptr = new (std::nothrow) char[max_vector_bytes];
  return initid->ptr == nullptr;
}

static void vector_addition_udf_deinit(UDF_INIT *initid) {
  delete[] initid->ptr;
  initid->ptr = nullptr;
}

const char *vector_addition_udf(UDF_INIT *initid, UDF_ARGS *args, char *vector_sum,
                          unsigned long *length, char *is_null, char *error) {

    *error = 0;
    *is_null = 0;

    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *error = 1;
      *is_null = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);

    unsigned long vector_length = 0;

    vector_sum = vector_addition(dim_vec1, vec1, vec2, initid->ptr, &vector_length);
    *length = vector_length;

    
    return const_cast<char *>(vector_sum);
}

// UDF to implement a vector subtraction function between two vectors

static bool vector_subtraction_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_subtraction",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  initid->max_length = max_vector_bytes;
  initid->ptr = new (std::nothrow) char[max_vector_bytes];
  return initid->ptr == nullptr;
}

static void vector_subtraction_udf_deinit(UDF_INIT *initid) {
  delete[] initid->ptr;
  initid->ptr = nullptr;
}

const char *vector_subtraction_udf(UDF_INIT *initid, UDF_ARGS *args, char *vector_sum,
                          unsigned long *length, char *is_null, char *error) {

    *error = 0;
    *is_null = 0;

    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *error = 1;
      *is_null = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);

    unsigned long vector_length = 0;

    vector_sum = vector_subtraction(dim_vec1, vec1, vec2, initid->ptr, &vector_length);
    *length = vector_length;


    return const_cast<char *>(vector_sum);
}

// UDF to implement a vector scalar multiplication function that multiplies each evelement of th vector by a scalar

static bool vector_scalar_multiplication_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_scalar_multiplication",
                "this function requires 2 parameters");
      return true;
  }
  if (args->arg_type[1] != REAL_RESULT && args->arg_type[1] != DECIMAL_RESULT) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_scalar_multiplication",
                "the second parameteter (scalar) must be a float or a decimal (example: 3.0e-0 or 3.0)");
      return true;
  }
  initid->maybe_null = true;
  initid->max_length = max_vector_bytes;
  initid->ptr = new (std::nothrow) char[max_vector_bytes];
  return initid->ptr == nullptr;
}

static void vector_scalar_multiplication_udf_deinit(UDF_INIT *initid) {
  delete[] initid->ptr;
  initid->ptr = nullptr;
}

const char *vector_scalar_multiplication_udf(UDF_INIT *initid, UDF_ARGS *args, char *vector_sum,
                          unsigned long *length, char *is_null, char *error) {

    *error = 0;
    *is_null = 0;

    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    if (dim_vec1 == UINT32_MAX) {
      error_msg_size();
      *error = 1;
      *is_null = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    double p = 0.0;    
    if (args->arg_type[1] == DECIMAL_RESULT) {
            p = atof(args->args[1]);
    } else {
            p = *((double *)args->args[1]);
    }

    unsigned long vector_length = 0;

    vector_sum = vector_scalar_multiplication(dim_vec1, vec1, p, initid->ptr, &vector_length);
    *length = vector_length;


    return const_cast<char *>(vector_sum);
}


// UDF to implement a vector dot product function of two vectors

static bool vector_dot_product_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_dot_product",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  initid->max_length = max_vector_bytes;
  initid->ptr = new (std::nothrow) char[max_vector_bytes];
  return initid->ptr == nullptr; 
}
      
static void vector_dot_product_udf_deinit(UDF_INIT *initid) {
  delete[] initid->ptr;
  initid->ptr = nullptr;
} 

const char *vector_dot_product_udf(UDF_INIT *initid, UDF_ARGS *args, char *vector_sum,
                          unsigned long *length, char *is_null, char *error) {
  
    *error = 0; 
    *is_null = 0;

    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *error = 1;
      *is_null = 1;
      return 0;
    }
      
    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);
    
    unsigned long vector_length = 0;
    
    vector_sum = vector_dot_product(dim_vec1, vec1, vec2, initid->ptr, &vector_length);
    *length = vector_length; 


    return const_cast<char *>(vector_sum);
}

// UDF to calculate the cross product of two 3-dimensional vectors 

static bool vector_cross_product_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_cross_product",
                "this function requires 2 parameters");
      return true;
  }
  initid->maybe_null = true;
  initid->max_length = max_vector_bytes;
  initid->ptr = new (std::nothrow) char[max_vector_bytes];
  return initid->ptr == nullptr;
}

static void vector_cross_product_udf_deinit(UDF_INIT *initid) {
  delete[] initid->ptr;
  initid->ptr = nullptr;
}

const char *vector_cross_product_udf(UDF_INIT *initid, UDF_ARGS *args, char *vector_sum,
                          unsigned long *length, char *is_null, char *error) {

    *error = 0;
    *is_null = 0;

    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *error = 1;
      *is_null = 1;
      return 0;
    }
    if (dim_vec1 != 3) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_cross_product",
                "this function requires only 3-dimensional vectors");
      *error = 1;
      *is_null = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);

    unsigned long vector_length = 0;

    vector_sum = vector_cross_product(dim_vec1, vec1, vec2, initid->ptr, &vector_length);
    *length = vector_length;


    return const_cast<char *>(vector_sum);
}

// UDF to calculate the norm (or length) of a vector

static bool vector_norm_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count != 1) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_norm",
                "this function requires 1 parameter");
      return true;
  }
  initid->maybe_null = true;
  return false;
}

static void vector_norm_udf_deinit(UDF_INIT *) {}

double vector_norm_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float norm = 0.0;
    *outp = 0;


    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    if (dim_vec1 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);

    norm = vector_norm(dim_vec1, vec1);

    return norm;
}

// UDF to calculate the angle between two vectors

static bool vector_angle_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2 || args->arg_count > 3) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_angle",
                "this function requires 2 or 3 parameters");
      return true;
  }

  initid->maybe_null = true;
  return false;
}

static void vector_angle_udf_deinit(UDF_INIT *) {}

double vector_angle_udf(UDF_INIT *, UDF_ARGS *args, unsigned char *outp,
                          unsigned char *) {

    float angle = 0.0;
    *outp = 0;

    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *outp = 1;
      return 0;
    }

    int output_format = 1; // 1 = radian, 2 = degrees

    if (args->arg_count == 3) {
      const auto *output_result = static_cast<const char *>(args->args[2]);
      if (strcasecmp(output_result, "RADIANS") == 0) {
	      output_format = 1;
      } else if (strcasecmp(output_result, "DEGREES") == 0) {
	      output_format = 2;
      } else {
	      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
			ER_UDF_ERROR, 0, "vector_angle",
			"the output format should be ('RADIANS' or 'DEGREES')");
	      *outp = 1;
	      return 0;
      }
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);

    angle = vector_angle(dim_vec1, vec1, vec2);

    if (output_format == 1) {
	    return angle;
    } else {
	    return angle * (180.0 / M_PI);
    }
}

// UDF to normalizes a vector to have unit length

static bool vector_normalize_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 1) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_normalize",
                "this function requires 1 parameter");
      return true;
  }

  initid->maybe_null = true;
  initid->max_length = max_vector_bytes;
  initid->ptr = new (std::nothrow) char[max_vector_bytes];
  return initid->ptr == nullptr;
}

static void vector_normalize_udf_deinit(UDF_INIT *initid) {
  delete[] initid->ptr;
  initid->ptr = nullptr;
}

const char *vector_normalize_udf(UDF_INIT *initid, UDF_ARGS *args, char *vector_sum,
                          unsigned long *length, char *is_null, char *error) {

    *error = 0;
    *is_null = 0;

    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    if (dim_vec1 == UINT32_MAX) {
      error_msg_size();
      *error = 1;
      *is_null = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);

    unsigned long vector_length = 0;

    vector_sum = vector_normalize(dim_vec1, vec1, initid->ptr, &vector_length);
    *length = vector_length;


    return const_cast<char *>(vector_sum);
}

// UDF to projects one vector onto another.

static bool vector_projection_udf_init(UDF_INIT *initid, UDF_ARGS *args, char *) {
  if (args->arg_count < 2) {
      mysql_error_service_emit_printf(mysql_service_mysql_runtime_error,
                ER_UDF_ERROR, 0, "vector_projection",
                "this function requires 2 parameter");
      return true;
  }

  initid->maybe_null = true;
  initid->max_length = max_vector_bytes;
  initid->ptr = new (std::nothrow) char[max_vector_bytes];
  return initid->ptr == nullptr;
}

static void vector_projection_udf_deinit(UDF_INIT *initid) {
  delete[] initid->ptr;
  initid->ptr = nullptr;
}

const char *vector_projection_udf(UDF_INIT *initid, UDF_ARGS *args, char *vector_sum,
                          unsigned long *length, char *is_null, char *error) {

    *error = 0;
    *is_null = 0;

    uint32_t dim_vec1 = get_dimensions(args->lengths[0], sizeof(float));
    uint32_t dim_vec2 = get_dimensions(args->lengths[1], sizeof(float));
    if (dim_vec1 != dim_vec2 || dim_vec1 == UINT32_MAX || dim_vec2 == UINT32_MAX) {
      error_msg_size();
      *error = 1;
      *is_null = 1;
      return 0;
    }

    float *vec1 = ((float *)args->args[0]);
    float *vec2 = ((float *)args->args[1]);

    unsigned long vector_length = 0;

    vector_sum = vector_projection(dim_vec1, vec1, vec2, initid->ptr, &vector_length);
    *length = vector_length;


    return const_cast<char *>(vector_sum);
}

} /* namespace udf_impl */


static mysql_service_status_t vector_community_service_init() {
  mysql_service_status_t result = 0;

  log_bi = mysql_service_log_builtins;
  log_bs = mysql_service_log_builtins_string;

  LogComponentErr(INFORMATION_LEVEL, ER_LOG_PRINTF_MSG, "initializing…");

  list = new udf_list();

  if (list->add_scalar("VECTOR_EUCLIDIAN_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_euclidian_distance_udf,
                       udf_impl::vector_euclidian_distance_udf_init,
                       udf_impl::vector_euclidian_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_DOTPRODUCT_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_dotproduct_distance_udf,
                       udf_impl::vector_dotproduct_distance_udf_init,
                       udf_impl::vector_dotproduct_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }
 
  if (list->add_scalar("VECTOR_COSINE_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_cosine_distance_udf,
                       udf_impl::vector_cosine_distance_udf_init,
                       udf_impl::vector_cosine_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_MANHATTAN_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_manhattan_distance_udf,
                       udf_impl::vector_manhattan_distance_udf_init,
                       udf_impl::vector_manhattan_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_CHEBYSHEV_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_chebyshev_distance_udf,
                       udf_impl::vector_chebyshev_distance_udf_init,
                       udf_impl::vector_chebyshev_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_JACCARD_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_jaccard_distance_udf,
                       udf_impl::vector_jaccard_distance_udf_init,
                       udf_impl::vector_jaccard_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_HAMMING_DISTANCE", Item_result::INT_RESULT,
                       (Udf_func_any)udf_impl::vector_hamming_distance_udf,
                       udf_impl::vector_hamming_distance_udf_init,
                       udf_impl::vector_hamming_distance_udf_deinit)) {
    delete list;
    
    return 1; /* failure: one of the UDF registrations failed */ 
  }

  if (list->add_scalar("VECTOR_BHATTACHARYYA_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_bhattacharyya_distance_udf,
                       udf_impl::vector_bhattacharyya_distance_udf_init,
                       udf_impl::vector_bhattacharyya_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_CANBERRA_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_canberra_distance_udf,
                       udf_impl::vector_canberra_distance_udf_init,
                       udf_impl::vector_canberra_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_HELLINGER_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_hellinger_distance_udf,
                       udf_impl::vector_hellinger_distance_udf_init,
                       udf_impl::vector_hellinger_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_MINKOWSKI_DISTANCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_minkowski_distance_udf,
                       udf_impl::vector_minkowski_distance_udf_init,
                       udf_impl::vector_minkowski_distance_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_KULLBACK_LEIBLER_DIVERGENCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_kullback_leibler_divergence_udf,
                       udf_impl::vector_kullback_leibler_divergence_udf_init,
                       udf_impl::vector_kullback_leibler_divergence_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_JENSEN_SHANNON_DIVERGENCE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_jensen_shannon_divergence_udf,
                       udf_impl::vector_jensen_shannon_divergence_udf_init,
                       udf_impl::vector_jensen_shannon_divergence_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */   
  }

  if (list->add_scalar("VECTOR_ADDITION", Item_result::STRING_RESULT,
                       (Udf_func_any)udf_impl::vector_addition_udf,
                       udf_impl::vector_addition_udf_init,
                       udf_impl::vector_addition_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_SUBTRACTION", Item_result::STRING_RESULT,
                       (Udf_func_any)udf_impl::vector_subtraction_udf,
                       udf_impl::vector_subtraction_udf_init,
                       udf_impl::vector_subtraction_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_SCALAR_MULTIPLICATION", Item_result::STRING_RESULT,
                       (Udf_func_any)udf_impl::vector_scalar_multiplication_udf,
                       udf_impl::vector_scalar_multiplication_udf_init,
                       udf_impl::vector_scalar_multiplication_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_DOT_PRODUCT", Item_result::STRING_RESULT,
                       (Udf_func_any)udf_impl::vector_dot_product_udf,
                       udf_impl::vector_dot_product_udf_init,
                       udf_impl::vector_dot_product_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_CROSS_PRODUCT", Item_result::STRING_RESULT,
                       (Udf_func_any)udf_impl::vector_cross_product_udf,
                       udf_impl::vector_cross_product_udf_init,
                       udf_impl::vector_cross_product_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_NORM", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_norm_udf,
                       udf_impl::vector_norm_udf_init,
                       udf_impl::vector_norm_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_ANGLE", Item_result::REAL_RESULT,
                       (Udf_func_any)udf_impl::vector_angle_udf,
                       udf_impl::vector_angle_udf_init,
                       udf_impl::vector_angle_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_NORMALIZE", Item_result::STRING_RESULT,
                       (Udf_func_any)udf_impl::vector_normalize_udf,
                       udf_impl::vector_normalize_udf_init,
                       udf_impl::vector_normalize_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  if (list->add_scalar("VECTOR_PROJECTION", Item_result::STRING_RESULT,
                       (Udf_func_any)udf_impl::vector_projection_udf,
                       udf_impl::vector_projection_udf_init,
                       udf_impl::vector_projection_udf_deinit)) {
    delete list;
    return 1; /* failure: one of the UDF registrations failed */
  }

  return result;
}

static mysql_service_status_t vector_community_service_deinit() {
  mysql_service_status_t result = 0;

  if (list->unregister()) return 1; /* failure: some UDFs still in use */

  delete list;

  LogComponentErr(INFORMATION_LEVEL, ER_LOG_PRINTF_MSG, "uninstalled.");

  return result;
}

BEGIN_COMPONENT_PROVIDES(vector_community_service)
END_COMPONENT_PROVIDES();

BEGIN_COMPONENT_REQUIRES(vector_community_service)
    REQUIRES_SERVICE(log_builtins),
    REQUIRES_SERVICE(log_builtins_string),
    REQUIRES_SERVICE(mysql_udf_metadata),
    REQUIRES_SERVICE(udf_registration),
    REQUIRES_SERVICE(mysql_runtime_error),
END_COMPONENT_REQUIRES();

/* A list of metadata to describe the Component. */
BEGIN_COMPONENT_METADATA(vector_community_service)
METADATA("mysql.author", "lefred"),
    METADATA("mysql.license", "GPL"), METADATA("mysql.dev", "lefred"),
END_COMPONENT_METADATA();

/* Declaration of the Component. */
DECLARE_COMPONENT(vector_community_service,
                  "mysql:vector_community_service")
vector_community_service_init,
    vector_community_service_deinit END_DECLARE_COMPONENT();

/* Defines list of Components contained in this library. Note that for now
  we assume that library will have exactly one Component. */
DECLARE_LIBRARY_COMPONENTS &COMPONENT_REF(vector_community_service)
    END_DECLARE_LIBRARY_COMPONENTS

