#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <algorithm>
using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";
imdb::imdb(const string& directory) {
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const {
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

imdb::~imdb() {
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

bool imdb::getCredits(const string& player, vector<film>& films) const { 
  // return false;
  // cout<<"\nPlayer: "<<player<<player.length()<<endl;

  int n = *(int*)actorFile;
  // cout<<"n: "<<n<<endl;

  int *start = (int*)actorFile + 1;
  int *end = (int*)actorFile + n;

  int *pos = lower_bound(start, end, player, [this](int x, string y) {
    char *x_data_address = (char*)(actorFile) + x;
    int i=0;
    string x_data = "";
    while(1) {
      char temp = *(x_data_address+i);
      if(temp=='\0') break;
      x_data += temp;
      i++;
    }
    return x_data < y;
  });
  
  char *pos_data_address = (char*)actorFile + *pos;
  int i=0;
  string pos_data = "";
  while(1) {
    char temp = *(pos_data_address+i);
    if(temp=='\0') break;
    pos_data += temp;
    i++;
  }
  // cout<<"pos_data: "<<pos_data<<pos_data.length()<<endl;
  if(pos_data != player) return false;

  if(i%2==0) i+=2;
  else i++;
  short num_movies = *(short*)(pos_data_address+i);
  // cout<<"num movies: "<<num_movies<<endl;

  i+=2;
  i+=(4-i%4)%4; // make it multiple of 4 i.e., skip other bytes

  while(num_movies--)
  {
    int movie_offset = *(int*)(pos_data_address+i);
    char *movie_address = (char*)movieFile + movie_offset;
    int j=0;
    string movie = "";
    while(1) {
      char temp = *(movie_address+j);
      if(temp=='\0') break;
      movie += temp;
      j++;
    }
    char year = *((char*)movie_address+j+1);
    // cout<<movie<<" : "<<int(year)+1900<<endl;
    i+=4;

    film t;
    t.title = movie;
    t.year = int(year)+1900;
    films.push_back(t);
  }

  return true; 
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 
  int num_movies = *(int*)movieFile;
  // cout<<"num_movies:"<<num_movies<<endl;

  int *start = (int*)movieFile + 1;
  int *end = (int*)movieFile + num_movies + 1;

  int *pos = lower_bound(start, end, movie, [this](int x, film y) {
    char *x_data_address = (char*)(movieFile) + x;
    string movie_title = string((char*)x_data_address);
    char year = *(x_data_address + movie_title.length() + 1);
    film t;
    t.title = movie_title;
    t.year = int(year)+1900;
    return t < y;
  });

  char *x_data_address = (char*)(movieFile) + *pos;
  string movie_title = string(x_data_address);
  char year = *(x_data_address + movie_title.length() + 1);
  film t;
  t.title = movie_title;
  t.year = int(year)+1900;
  if(!(t==movie)) return false;

  // test code
  // char *x_data_address = (char*)(movieFile) + *pos;
  // string movie_title = string((char*)x_data_address);
  // char year = *(x_data_address + movie_title.length() + 1);
  // cout<<"movie:"<<movie_title<<" ("<<int(year)+1900<<")\n";

  char *data_address = (char*)(movieFile) + *pos;
  int offset = string((char*)data_address).length() + 1 + 1;
  // cout<<"offset: "<<offset<<endl;
  if(offset%2!=0) offset++;
  short num_actors = *(short*)(data_address + offset);
  offset += 2;
  // cout<<"num_actors: "<<num_actors<<endl;
  offset+=(4-offset%4)%4; // make it multiple of 4 i.e., skip other bytes

  while(num_actors--) {
    int actor_offset = *(int*)(data_address + offset);
    string actor_name = string((char*)actorFile + actor_offset);
    // cout<<"Actor Name:"<< actor_name<<endl;
    players.push_back(actor_name);
    offset+=4;
    // break;
  }
 
  return true; 
}

const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info) {
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info) {
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
