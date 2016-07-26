#include "fargo3d.h"

void SelectDevice(int myrank){
#ifdef GPU
  char hostname[1024];
  int device, i;
  boolean ChooseDeviceForMe = YES;
  struct cudaDeviceProp prop;
  gethostname(hostname,1023);
  /* You may choose below a device selection for a specific
     platform. The fall back is that the best device with compute
     capabilities at least 2.x is chosen automatically for you. When
     you run a MPI/GPU configuration, you should avoid to use this
     default, as there is a risk that two distinct processes access
     the same device. You should rather define a device selection rule
     below. */
  Check_CUDA_Blocks_Consistency ();
  /* Custom device selection rules depending on host name */
  /* The user may edit or had his own rules  */
  if (strcmp(hostname, "tesla") == 0) {
    device = 1-(myrank % 2);
    ChooseDeviceForMe = NO;
  }
  if (strncmp(hostname, "com", 3) == 0) {
    device = myrank % 4;
    ChooseDeviceForMe = NO;
  }
  if (DeviceManualSelection >= 0) {
    ChooseDeviceForMe = NO;
    device = DeviceManualSelection;
    if (myrank == 1) fprintf (stderr, "WARNING ! You have selected the device manually but your run is MPI\n");
  }
  if (ChooseDeviceForMe) {
    memset(&prop, 0, sizeof(struct cudaDeviceProp));
    prop.major = 2;
    cudaChooseDevice(&device,&prop);
  }
  cudaGetDeviceProperties(&prop,device);
  cudaSetDevice(device);
  cudaDeviceReset();
  for (i = 0; i<strlen(prop.name); i++){ //Name to upper case
    prop.name[i] = (char)toupper(prop.name[i]);
  }
  printf("\n%s\n","=========================");
  printf("%s%d\n","PROCESS NUMBER       ",myrank);
  printf("%s%d\n","RUNNING ON DEVICE Nº ",device);
  printf("%s\n",prop.name);
  printf("%s%d%s%d\n", "COMPUTE CAPABILITY: ", prop.major,".",prop.minor);
  printf("%s%ld%s\n","VIDEO RAM MEMORY: ", prop.totalGlobalMem/1000000000 ," GB");
  printf("%s\n\n","=========================");

  if (fabs((prop.major)) > 100) {
    printf("Error!!! Verify your device, something is wrong. Try to use manual device selection (-D switch)\n\n");
    exit(EXIT_FAILURE);
  }
#endif
}

void EarlyDeviceSelection () {
#ifdef GPU
  char hostname[1024];
  int local_rank, device, mydevice=-1;
  char MpiEnvVariable[MAXLINELENGTH];
  char PeNbString[MAXLINELENGTH];
  char s[MAXLINELENGTH];
  FILE *devfile;
  int c=0;
  gethostname(hostname,1022);
  sprintf (MpiEnvVariable, xstr(ENVRANK));
  if ((strcmp(MpiEnvVariable, "ENVRANK") == 0) || (strcmp(MpiEnvVariable, "") == 0)) {
    printf ("ERROR ====\n");
    printf ("You have built the code for a CUDA aware MPI implementation\n");
    printf ("in order to have efficient GPU-GPU communications.\n");
    printf ("But you have not provided the name of the environment variable\n");
    printf ("that is required to start the code. This variable depends on your\n");
    printf ("MPI implementation. For instance, for OpenMPI, it is called\n");
    printf ("OMPI_COMM_WORLD_LOCAL_RANK. For Mvapich2, it is NV2_COMM_WORLD_LOCAL_RANK, etc.\n");
    printf ("You must define the name of this variable in src/makefile\n");
    printf ("with the variable ENVRANK, for your specific platform. Have a look\n");
    printf ("at the examples provided in the makefile to adapt them to your situation.\n");
    exit(EXIT_FAILURE);
  }
  if (getenv(MpiEnvVariable) == NULL) {
    printf ("I cannot find a valid (local) rank for the process.\n");
    printf ("Did you launch the run with 'mpirun' or equivalent ?\n");
    printf ("CUDA-aware MPI jobs MUST be launched through 'mpirun'\n");
    printf ("even if only one process is launched.\n");
    printf ("It is also possible that the executable was built for a\n");
    printf ("given architecture (e.g. OpenMPI), then run with\n");
    printf ("another one (e.g. MVAPICH2)\n");
    printf ("I must exit\n");
    exit(1);
  }
  local_rank = atoi(getenv(MpiEnvVariable));
  device = local_rank;
  if (DeviceFileSpecified == YES) {
    devfile = fopen(DeviceFile, "r");
    if (devfile == NULL) {
      fprintf (stderr, "ERROR: cannot open device file %s\n", DeviceFile);
      exit (1);
    }
    while (fgets(s, MAXLINELENGTH-1, devfile) != NULL ) {
      if (strncmp (s, hostname, strlen(hostname)) == 0) {
	if (c == local_rank) {
	  mydevice = atoi(s+strcspn(s,"=:/")+1);
	  printf ("Process on host %s w/ local rank %d runs on device %d\n",\
		  hostname, local_rank, mydevice);
	}
	c++;
      }
    }
    fclose(devfile);
    if (mydevice < 0) {
      fprintf(stderr, "Process on %s w/ local rank %d did not find its matching device in %s. Aborted.\n", hostname, local_rank, DeviceFile);
      exit(EXIT_FAILURE);
    }
    device = mydevice;
  }
  if (DeviceManualSelection >= 0) {
    device = DeviceManualSelection;
    if (local_rank == 1) fprintf (stderr, "WARNING ! You have selected the device manually but your run is MPI\n");
  }
  cudaSetDevice (device);
  printf ("Process with local rank %d on host %s\n", local_rank, hostname);
#endif
}


