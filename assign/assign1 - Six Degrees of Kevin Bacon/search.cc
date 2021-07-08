#include <vector>
#include <iostream>
#include <queue>
#include <set>
#include <map>
#include "imdb.h"
#include "path.h"
using namespace std;

static const int kWrongArgumentCount = 1;
static const int kDatabaseNotFound = 2;

void search_bfs(const imdb& db, string &src, string &dest) {
  queue<string> q;
  set<string> visited_actor;
  set<film> visited_movie;
  map<string, pair<string, film>> parent;
  bool found = false;

  q.push(src);
  visited_actor.insert(src);
  parent[src] = {src, film()};

  while(!q.empty() && found==false) {

    string star = q.front();
    q.pop();
    // cout<<"Src: "<<star<<endl;

    vector<film> movies;
    if (!db.getCredits(star, movies) || movies.size() == 0) {
      cout << "We're sorry, but " << star << " doesn't appear to be in our database." << endl;
      return;
    }

    for (int i = 0; i < (int) movies.size()  && found==false; i++) {
      const film& movie = movies[i];
      if(visited_movie.find(movie) == visited_movie.end()) {
        visited_movie.insert(movie);
      }
      else {
        continue;
      }
      vector<string> cast;
      db.getCast(movie, cast);
      for (int j = 0; j < (int) cast.size()  && found==false; j++) {
        const string& costar = cast[j];
        // cout<<"Conn: "<<costar<<endl;
        if(visited_actor.find(costar) != visited_actor.end()) {
          continue;
        }
        else {
          visited_actor.insert(costar);
          parent[costar] = {star, movie};
          q.push(costar);
        }
        if(costar==dest) {
          found = true;
        }
      }
    }
    // break;
  }

  //TODO: Fix the issue with printing
  path p(dest);
  while(src != dest) {
    string star = parent[dest].first;
    film movie = parent[dest].second;
	// cout<<star<<" "<<movie.title<<endl;
    p.addConnection(movie, star);
    dest = parent[dest].first;
  }
  p.reverse();
  cout<<p;
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    cerr << "Usage: " << argv[0] << " <actor> <actor>" << endl;
    return kWrongArgumentCount;
  }

  imdb db(kIMDBDataDirectory);
  if (!db.good()) {
    cerr << "Data directory not found!  Aborting..." << endl; 
    return kDatabaseNotFound;
  }

  string actor1 = argv[1];
  string actor2 = argv[2];
  //cout<<"actor1 = "<<actor1<<"\n";
  //cout<<"actor2 = "<<actor2<<"\n";

  search_bfs(db, actor1, actor2);
  cout<<endl;


  return 0;
}
