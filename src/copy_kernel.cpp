extern "C" 
{
void copy_kernel (unsigned int *db, unsigned int *value, unsigned int value_size, 
    unsigned int seq, const int direction) {
#pragma HLS INTERFACE m_axi port = db offset = slave
#pragma HLS INTERFACE m_axi port = value offset = slave

  unsigned int index = seq * value_size;

  if (direction == 0) {
  copy_h2d:
    for (unsigned int i = index; i < value_size; i++ ){
#pragma HLS pipeline
      db[i] = value[i];
    }
  }
  else {
  copy_d2h:
    for (unsigned int i = index; i < value_size; i++ ){
#pragma HLS pipeline
      value[i] = db[i];
    }

  }
}
}
