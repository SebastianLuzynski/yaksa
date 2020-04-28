/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <assert.h>

int yaksi_create_subarray(int ndims, const int *array_of_sizes, const int *array_of_subsizes,
                          const int *array_of_starts, yaksa_subarray_order_e order,
                          yaksi_type_s * intype, yaksi_type_s ** newtype)
{
    int rc = YAKSA_SUCCESS;

    yaksi_type_s *outtype;
    rc = yaksi_type_alloc(&outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    outtype->kind = YAKSI_TYPE_KIND__SUBARRAY;
    outtype->tree_depth = intype->tree_depth + 1;
    outtype->alignment = intype->alignment;

    /* create a series of hvectors for the subarray, but store the lb
     * and ub separately.  this is because subarray allows the buffer
     * to point to the lb for pack/unpack operations, but that's not
     * true for hvectors (even when resized). */

    outtype->u.subarray.ndims = ndims;

    /* handle the first dimension separately because it really is a
     * contig, rather than a vector */

    intptr_t stride = intype->extent;

    yaksi_type_s *current, *next;
    if (order == YAKSA_SUBARRAY_ORDER__C) {
        rc = yaksi_create_contig(array_of_subsizes[ndims - 1], intype, &current);
        YAKSU_ERR_CHECK(rc, fn_fail);

        for (int i = ndims - 2; i >= 0; i--) {
            stride *= array_of_sizes[i + 1];
            rc = yaksi_create_hvector(array_of_subsizes[i], 1, stride, current, &next);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = yaksi_free(current);
            YAKSU_ERR_CHECK(rc, fn_fail);

            current = next;
        }
    } else {
        rc = yaksi_create_contig(array_of_subsizes[0], intype, &current);
        YAKSU_ERR_CHECK(rc, fn_fail);

        for (int i = 1; i < ndims; i++) {
            stride *= array_of_sizes[i - 1];
            rc = yaksi_create_hvector(array_of_subsizes[i], 1, stride, current, &next);
            YAKSU_ERR_CHECK(rc, fn_fail);

            rc = yaksi_free(current);
            YAKSU_ERR_CHECK(rc, fn_fail);

            current = next;
        }
    }

    uintptr_t extent = intype->extent;
    for (int i = 0; i < ndims; i++)
        extent *= array_of_sizes[i];

    rc = yaksi_create_resized(current, 0, extent, &next);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_free(current);
    YAKSU_ERR_CHECK(rc, fn_fail);

    outtype->u.subarray.primary = next;

    outtype->lb = 0;
    outtype->ub = extent;
    outtype->extent = extent;

    outtype->size = intype->size;
    outtype->true_lb = intype->true_lb;
    outtype->true_ub = intype->true_ub;

    for (int i = 0; i < ndims; i++) {
        outtype->size *= array_of_subsizes[i];

        intptr_t true_lb = array_of_starts[i] * intype->extent;
        intptr_t true_ub = (array_of_starts[i] + array_of_subsizes[i] - 1) * intype->extent;
        if (order == YAKSA_SUBARRAY_ORDER__C) {
            for (int j = i + 1; j < ndims; j++) {
                true_lb *= array_of_sizes[j];
                true_ub *= array_of_sizes[j];
            }
        } else {
            for (int j = 0; j < i; j++) {
                true_lb *= array_of_sizes[j];
                true_ub *= array_of_sizes[j];
            }
        }
        outtype->true_lb += true_lb;
        outtype->true_ub += true_ub;
    }

    /* detect if the outtype is contiguous */
    if (intype->is_contig && outtype->ub == outtype->size) {
        outtype->is_contig = true;
        for (int i = 0; i < ndims; i++) {
            if (array_of_starts[i]) {
                outtype->is_contig = false;
                break;
            }
            if (array_of_subsizes[i] != array_of_sizes[i]) {
                outtype->is_contig = false;
                break;
            }
        }
    } else {
        outtype->is_contig = false;
    }

    outtype->num_contig = outtype->u.subarray.primary->num_contig;

    yaksur_type_create_hook(outtype);
    *newtype = outtype;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksa_create_subarray(int ndims, const int *array_of_sizes, const int *array_of_subsizes,
                          const int *array_of_starts, yaksa_subarray_order_e order,
                          yaksa_type_t oldtype, yaksa_type_t * newtype)
{
    int rc = YAKSA_SUCCESS;

    assert(yaksi_global.is_initialized);

    if (ndims == 0) {
        *newtype = YAKSA_TYPE__NULL;
        goto fn_exit;
    }

    yaksi_type_s *intype;
    rc = yaksi_type_get(oldtype, &intype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    yaksi_type_s *outtype;
    rc = yaksi_create_subarray(ndims, array_of_sizes, array_of_subsizes, array_of_starts, order,
                               intype, &outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *newtype = outtype->id;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
