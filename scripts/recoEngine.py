#!/usr/bin/env python3
import sys
import pandas as pd
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.metrics.pairwise import cosine_similarity
import json

def process_library_data(song_id, album_id, library_data):
    
    if not library_data:
        print("no data to process.", file=sys.stderr)
        return None, None
    
    # gen dataframe from library data
    df = pd.DataFrame(library_data, columns=["id", "title", "artist", "genre", "album", "album_id", "filePath"])
    
    # change values to int
    df["id"] = df["id"].astype(int)
    df["album_id"] = df["album_id"].astype(int)
    
    # get current song
    current_song = df[df["id"] == song_id]
    if current_song.empty:
        print(f"Song ID {song_id} not found in data.", file=sys.stderr)
        return None, None
    
    current_artist = current_song["artist"].values[0]
    current_genre = current_song["genre"].values[0]
    
    # get artist reccomendation based on genre similarity
    artist_recommendations = get_artist_recommendations(df, current_artist, current_genre)
    
    # genre recommendations using TF-IDF and cosine similarity
    genre_recommendations = get_genre_recommendations(df, song_id, current_genre)
    
    return artist_recommendations, genre_recommendations

def get_artist_recommendations(df, current_artist, current_genre):

    artist_data = df.drop_duplicates(subset=["artist"])
    
    # filter current artist and genre
    other_artists = artist_data[
        (artist_data["artist"] != current_artist) & 
        (artist_data["genre"] == current_genre)
    ]
    
    if other_artists.empty:
        # expand search to all artists
        other_artists = artist_data[artist_data["artist"] != current_artist]
    
    # get top 5 artists
    top_artists = other_artists.head(5)
    
    return top_artists[["artist", "genre"]].to_dict(orient="records")

def get_genre_recommendations(df, song_id, current_genre):
    
    # combine for tdf vectorizsation
    df["features"] = df["title"] + " " + df["artist"] + " " + df["genre"] + " " + df["album"]
    
    # form matrix
    vectorizer = TfidfVectorizer(stop_words='english')
    tfidf_matrix = vectorizer.fit_transform(df["features"])
    
    # consine similarity
    cosine_sim = cosine_similarity(tfidf_matrix, tfidf_matrix)
    
    #get song index
    song_index = df.index[df["id"] == song_id][0]
    
    similarity_scores = list(enumerate(cosine_sim[song_index]))
    
    # sort by similarity 
    similarity_scores = sorted(similarity_scores, key=lambda x: x[1], reverse=True)
    
    # top 10 songs
    similar_songs_indices = [i[0] for i in similarity_scores if i[0] != song_index][:10]
    
    return df.iloc[similar_songs_indices][["id", "title", "artist", "genre", "album"]].to_dict(orient="records")

def main():
    # retrieve cmd args 
    song_id = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    album_id = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    
    # Read data from stdin
    lines = [line.strip() for line in sys.stdin if line.strip()]
    library_data = []
    
    for line in lines:
        parts = line.split("|")
        if len(parts) >= 7:
            library_data.append(parts)
    
    artist_recommendations, genre_recommendations = process_library_data(song_id, album_id, library_data)
    
    # output results
    results = {
        "artist_recommendations": artist_recommendations,
        "genre_recommendations": genre_recommendations
    }
    
    print("RECOMMENDATIONS_BEGIN") #macro to let C++ program know whats goin off 
    print(json.dumps(results, indent=2))
    print("RECOMMENDATIONS_END")

if __name__ == "__main__":
    main()