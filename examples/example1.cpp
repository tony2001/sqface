#include "sqface.h"

int main (int argc, char **argv) {
  char *filename_i, *filename_i_txt;
  char *filename_o;
  if (argc < 3) {
    printf("%s [filename_i] [filename_o] [haar_i_txt]\n", argv[0]);
    return -1;
  }
  filename_i = argv[1];
  filename_o = argv[2];
  if(argc > 3)
    filename_i_txt = argv[3];
  else
    filename_i_txt = "haarcascade_frontalface_alt.xml";
  TFaceRecognizer *Rec = new TFaceRecognizer();
  Rec->LoadImage(filename_i);
  Rec->LoadCascadeXML(filename_i_txt);
  Rec->Recognize(1.2);
  printf("%d x %d\n", Rec->GetImageWidth(), Rec->GetImageHeight());
  Rec->SaveImage(filename_o);
  Rec->UnloadImage();
  return 0;
}
