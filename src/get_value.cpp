extern "C" 
{
void get_value (unsigned int *db, unsigned int *value, unsigned int value_size) {
set_op:
  for (unsigned int i = 0; i < value_size; i++ ){
    #pragma HLS PIPELINE II=1
    value[i] = db[i];
  }
}
}
