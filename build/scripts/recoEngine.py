import sys
import pandas as pd
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.metrics.pairwise import cosine_similarity

def train_and_recommend(song_id, user_music_data):
    if not user_music_data:
        print("No data to process.", file=sys.stderr)
        return pd.DataFrame()

    # Load user music data into a DataFrame
    df = pd.DataFrame(user_music_data, columns=["title", "artist", "genre"])

    if df.empty:
        print("DataFrame is empty.", file=sys.stderr)
        return pd.DataFrame()

    # Combine features for similarity calculation
    df["combined_features"] = df["title"] + " " + df["artist"] + " " + df["genre"]

    # Vectorize the combined features
    vectorizer = TfidfVectorizer()
    feature_matrix = vectorizer.fit_transform(df["combined_features"])

    # Calculate cosine similarity
    similarity_matrix = cosine_similarity(feature_matrix)

    # Get recommendations for the given song_id
    recommendations = similarity_matrix[song_id].argsort()[-6:-1][::-1]  # Top 5 recommendations
    return df.iloc[recommendations]

if __name__ == "__main__":
    for line in sys.stdin:
        print(f"Echo: {line.strip()}")