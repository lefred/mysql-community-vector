# MySQL Community Vector Component

A MySQL component that adds distance, divergence and arithmetic functions for
the `VECTOR` data type (MySQL 9.x).

## Build

Link (or copy) this directory into the MySQL source tree as
`components/community_vector` and build the target:

```bash
cd mysql-server
ln -s ../../mysql-community-vector components/community_vector
cd <build-dir>
cmake .. <your usual options>
make component_community_vector
```

The resulting `component_community_vector.so` is written to
`plugin_output_directory/`. Copy it to the server's `plugin_dir`.

## Install / uninstall

```sql
INSTALL COMPONENT "file://component_community_vector";

UNINSTALL COMPONENT "file://component_community_vector";
```

## Conventions

* Vector arguments are `VECTOR` values, for example `STRING_TO_VECTOR('[1,2,3]')`
  or a `VECTOR` column.
* When a function takes two vectors, both must have the same number of
  dimensions. Otherwise the function returns `NULL` and raises an error.
* Functions that return a vector return a binary `VECTOR` value. Wrap them with
  `VECTOR_TO_STRING()` to read the result.

Most examples below use these two vectors:

```sql
SET @a = STRING_TO_VECTOR('[1,2,3]');
SET @b = STRING_TO_VECTOR('[4,5,6]');
```

The example results were computed with the component's own code. The server
may print a few more or fewer trailing digits for floating-point results.

## Distance functions

### VECTOR_EUCLIDIAN_DISTANCE(v1, v2)

Straight-line (L2) distance: `sqrt(Σ (v1[i] - v2[i])²)`.

```sql
SELECT VECTOR_EUCLIDIAN_DISTANCE(@a, @b);
-- 5.196152210235596
```

### VECTOR_DOTPRODUCT_DISTANCE(v1, v2)

Dot product of the two vectors: `Σ v1[i] * v2[i]`.

```sql
SELECT VECTOR_DOTPRODUCT_DISTANCE(@a, @b);
-- 32
```

### VECTOR_COSINE_DISTANCE(v1, v2)

`1 - cosine_similarity`. Returns an error if one of the vectors is the zero
vector.

```sql
SELECT VECTOR_COSINE_DISTANCE(@a, @b);
-- 0.025368154048919678
```

### VECTOR_MANHATTAN_DISTANCE(v1, v2)

L1 distance: `Σ |v1[i] - v2[i]|`.

```sql
SELECT VECTOR_MANHATTAN_DISTANCE(@a, @b);
-- 9
```

### VECTOR_CHEBYSHEV_DISTANCE(v1, v2)

Largest difference in any dimension: `max |v1[i] - v2[i]|`.

```sql
SELECT VECTOR_CHEBYSHEV_DISTANCE(@a, @b);
-- 3
```

### VECTOR_JACCARD_DISTANCE(v1, v2)

Treats each dimension as set membership (non-zero = present):
`1 - |intersection| / |union|`.

```sql
SELECT VECTOR_JACCARD_DISTANCE(STRING_TO_VECTOR('[1,0,1,1]'),
                               STRING_TO_VECTOR('[1,1,0,1]'));
-- 0.5
```

### VECTOR_HAMMING_DISTANCE(v1, v2)

Number of dimensions where the values differ. Returns an integer.

```sql
SELECT VECTOR_HAMMING_DISTANCE(STRING_TO_VECTOR('[1,0,1,1]'),
                               STRING_TO_VECTOR('[1,1,0,1]'));
-- 2
```

### VECTOR_CANBERRA_DISTANCE(v1, v2)

`Σ |v1[i] - v2[i]| / (|v1[i]| + |v2[i]|)`. Dimensions where both values are 0 are skipped.

```sql
SELECT VECTOR_CANBERRA_DISTANCE(@a, @b);
-- 1.3619048595428467
```

### VECTOR_MINKOWSKI_DISTANCE(v1, v2, p)

Generalized distance `(Σ |v1[i] - v2[i]|^p)^(1/p)`. `p` must be a decimal or
float literal (for example `3.0` or `3.0e0`). With `p = 1` this is the Manhattan
distance, and with `p = 2` it is the Euclidean distance.

```sql
SELECT VECTOR_MINKOWSKI_DISTANCE(@a, @b, 3.0);
-- 4.326748847961426
```

## Probability distribution functions

These functions expect vectors that represent probability distributions:
non-negative values that sum to 1.

```sql
SET @p = STRING_TO_VECTOR('[0.1,0.4,0.5]');
SET @q = STRING_TO_VECTOR('[0.2,0.3,0.5]');
```

### VECTOR_BHATTACHARYYA_DISTANCE(p, q)

`-ln(Σ sqrt(p[i] * q[i]))`.

```sql
SELECT VECTOR_BHATTACHARYYA_DISTANCE(@p, @q);
-- 0.012243108823895454
```

### VECTOR_HELLINGER_DISTANCE(p, q)

`sqrt(Σ (sqrt(p[i]) - sqrt(q[i]))²) / sqrt(2)`. The result is between 0 and 1.

```sql
SELECT VECTOR_HELLINGER_DISTANCE(@p, @q);
-- 0.11031083762645721
```

### VECTOR_KULLBACK_LEIBLER_DIVERGENCE(p, q)

`Σ p[i] * ln(p[i] / q[i])`. Dimensions where either value is 0 are skipped.
The result is not symmetric: `KL(p, q) ≠ KL(q, p)`.

```sql
SELECT VECTOR_KULLBACK_LEIBLER_DIVERGENCE(@p, @q);
-- 0.04575808718800545
```

### VECTOR_JENSEN_SHANNON_DIVERGENCE(p, q)

Symmetric, smoothed version of Kullback-Leibler:
`½ KL(p, m) + ½ KL(q, m)` where `m = ½ (p + q)`.

```sql
SELECT VECTOR_JENSEN_SHANNON_DIVERGENCE(@p, @q);
-- 0.012078598141670227
```

## Vector arithmetic

### VECTOR_ADDITION(v1, v2)

Element-wise sum.

```sql
SELECT VECTOR_TO_STRING(VECTOR_ADDITION(@a, @b));
-- [5.00000e+00,7.00000e+00,9.00000e+00]
```

### VECTOR_SUBTRACTION(v1, v2)

Element-wise difference `v1 - v2`.

```sql
SELECT VECTOR_TO_STRING(VECTOR_SUBTRACTION(@a, @b));
-- [-3.00000e+00,-3.00000e+00,-3.00000e+00]
```

### VECTOR_SCALAR_MULTIPLICATION(v, scalar)

Multiplies each element by `scalar`. `scalar` must be a decimal or float
literal (for example `2.5` or `2.5e0`).

```sql
SELECT VECTOR_TO_STRING(VECTOR_SCALAR_MULTIPLICATION(@a, 2.5));
-- [2.50000e+00,5.00000e+00,7.50000e+00]
```

### VECTOR_DOT_PRODUCT(v1, v2)

Element-wise (Hadamard) product. To get the scalar dot product, use
`VECTOR_DOTPRODUCT_DISTANCE()`.

```sql
SELECT VECTOR_TO_STRING(VECTOR_DOT_PRODUCT(@a, @b));
-- [4.00000e+00,1.00000e+01,1.80000e+01]
```

### VECTOR_CROSS_PRODUCT(v1, v2)

Cross product. Only defined for 3-dimensional vectors.

```sql
SELECT VECTOR_TO_STRING(VECTOR_CROSS_PRODUCT(STRING_TO_VECTOR('[1,0,0]'),
                                             STRING_TO_VECTOR('[0,1,0]')));
-- [0.00000e+00,0.00000e+00,1.00000e+00]
```

### VECTOR_NORM(v)

Length (L2 norm) of the vector.

```sql
SELECT VECTOR_NORM(@a);
-- 3.7416574954986572
```

### VECTOR_NORMALIZE(v)

Scales the vector to unit length. Returns an error for the zero vector.

```sql
SELECT VECTOR_TO_STRING(VECTOR_NORMALIZE(@a));
-- [2.67261e-01,5.34522e-01,8.01784e-01]
```

### VECTOR_ANGLE(v1, v2 [, 'RADIANS' | 'DEGREES'])

Angle between two vectors. The default unit is radians.

```sql
SELECT VECTOR_ANGLE(@a, @b);
-- 0.22572623193264008

SELECT VECTOR_ANGLE(@a, @b, 'DEGREES');
-- 12.933160415131429
```

### VECTOR_PROJECTION(v1, v2)

Projection of `v1` onto `v2`: `(v1·v2 / |v2|²) * v2`.

```sql
SELECT VECTOR_TO_STRING(VECTOR_PROJECTION(@a, @b));
-- [1.66234e+00,2.07792e+00,2.49351e+00]
```

## Example: nearest neighbours

```sql
CREATE TABLE docs (
  id INT PRIMARY KEY,
  embedding VECTOR(3)
);

INSERT INTO docs VALUES
  (1, STRING_TO_VECTOR('[1,2,3]')),
  (2, STRING_TO_VECTOR('[4,5,6]')),
  (3, STRING_TO_VECTOR('[1,2,2.5]'));

SELECT id, VECTOR_COSINE_DISTANCE(embedding, STRING_TO_VECTOR('[1,2,3]')) AS dist
  FROM docs
 ORDER BY dist
 LIMIT 2;
```

## License

GPL
