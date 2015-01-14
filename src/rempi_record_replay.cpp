#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "mpi.h"

#include "rempi_record_replay.h"
#include "rempi_record.h"
#include "rempi_config.h"
#include "rempi_err.h"
#include "rempi_util.h"

#define REMPI_COMM_ID_LENGTH (128) // TODO: 1 byte are enough, but 1 byte causes segmentation fault ...

char next_comm_id_to_assign = 15; //TODO: Mutex for Hybrid MPI

int rempi_record_replay_init(int *argc, char ***argv)
{
  int return_val;
  int rank;
  char comm_id[REMPI_COMM_ID_LENGTH];
  comm_id[0] = next_comm_id_to_assign++; // Use only first 1 byte

  PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
  PMPI_Comm_set_name(MPI_COMM_WORLD, comm_id);
  rempi_err_init(rank);
  rempi_set_configuration(argc, argv);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    rempi_record_init(argc, argv, rank);
  } else {
    //TODO: Check if inputs are same as values in recording run
    //TODO: Check if this is same run as the last recording run
    rempi_replay_init(argc, argv, rank);
  }

  return return_val;
}

int rempi_record_replay_irecv(
	  void *buf, 
	  int count, 
	  MPI_Datatype datatype,
	  int source,
	  int tag,
	  MPI_Comm comm,
	  MPI_Request *request)
{
  int return_val;
  char comm_id[REMPI_COMM_ID_LENGTH];
  int comm_id_int;
  int resultlen;
  PMPI_Comm_get_name(MPI_COMM_WORLD, comm_id, &resultlen);

  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    //    return_val = PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
    //TODO: Get Datatype,
    rempi_record_irecv(buf, count, (int)(datatype), source, tag, (int)comm_id[0], request);
  } else {
    /*TODO: Really need datatype ??*/
    rempi_replay_irecv(buf, count, (int)(datatype), source, tag, (int)comm_id[0], request);
  }
  return return_val;
}

int rempi_record_replay_test(
    MPI_Request *request, /*TODO: use MPI_Request request insted of *request*/
    int *flag,
    MPI_Status *status)
{
  int return_val;
  status->MPI_SOURCE = 0;
  status->MPI_TAG    = 0;
  return_val = PMPI_Test(request, flag, status);
 
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    /*If recoding mode, record the test function*/
    /*TODO: change froi void* to MPI_Request*/
    rempi_record_test(request, flag, status->MPI_SOURCE, status->MPI_TAG);
  } else {
    int recorded_source, recorded_tag, recorded_flag;

    rempi_replay_test(request, *flag, status->MPI_SOURCE, status->MPI_TAG,
		      &recorded_flag, &recorded_source, &recorded_tag);
    /*Set next recorded and matched source, and tag*/
    status->MPI_SOURCE = recorded_source;
    status->MPI_TAG    = recorded_tag;
    *flag              = recorded_flag;

  }
  return return_val;
}

int rempi_record_replay_testsome(
	     int incount, 
	     MPI_Request array_of_requests[],
	     int *outcount, 
	     int array_of_indices[], 
	     MPI_Status array_of_statuses[])
{
  int return_val;
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    return_val = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    rempi_record_testsome(incount, (void**)(array_of_requests), (int*)(outcount), array_of_indices, (void**)array_of_statuses);
  } else {
    // TODO: replay
    // rempi_replay_test(buf, count, (int)(&datatype), source, tag, 0, (void*)request);
  }
  return return_val;
}

int rempi_record_replay_finalize(void)
{
  int return_val;
  if (rempi_mode == REMPI_ENV_REMPI_MODE_RECORD) {
    return_val = rempi_record_finalize();
  } else {
    return_val = rempi_replay_finalize();
  }
  return return_val;

}