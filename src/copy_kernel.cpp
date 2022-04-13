extern "C" 
{
void copy_kernel (unsigned int *db, unsigned int *value, unsigned int value_size, 
    const int direction) {
#pragma HLS INTERFACE m_axi port = db offset = slave
#pragma HLS INTERFACE m_axi port = value offset = slave

  if (direction == 0) {
  copy_h2d:
    for (unsigned int i = 0; i < value_size; i++ ){
      db[i] = value[i];
    }
  }
  else {
  copy_d2h:
    for (unsigned int i = 0; i < value_size; i++ ){
      value[i] = db[i];
    }

  }
}
}
