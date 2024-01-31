import pandas as pd

# Load the first CSV file
df_wrapper = pd.read_csv("WrapperResults.csv")

# Filter the dataframe for the specified rows
filtered_df = df_wrapper[df_wrapper['PREPROCESSING'].isin(['JUST_DFS', 'JUST_INIT'])]

# Initialize an empty list to store dataframes
dfs = []

# Process rows for MS-BFS_GRAFT with JUST_INIT
ms_bfs_graft_init_rows = filtered_df[
    (filtered_df['INITALGO'] == 'MS-BFS_GRAFT') & 
    (filtered_df['PREPROCESSING'] == 'JUST_INIT')]

for _, row in ms_bfs_graft_init_rows.iterrows():
    if row['INITALGO'] == 'MS-BFS_GRAFT':
        solver = 'MS-BFS_GRAFT'
        n_threads = 16
        if row['File'] == 'rgg_n_2_24_s0.mtx':
            n_threads = 8  # Adjust the number of threads based on the specific case

        dfs.append(pd.DataFrame({
            'File': [row['File']],
            'Solver': [solver],
            'NThreads': [n_threads],
            'Time': [row['INIT_TIME(S)']],
            'M': [row['INIT_M']]
        }))

# Process rows for Kececioglu
kececioglu_rows = filtered_df[
    (filtered_df['INITALGO'] == 'MS-BFS_GRAFT') & 
    (filtered_df['PREPROCESSING'] == 'JUST_DFS')]

for _, row in kececioglu_rows.iterrows():
    dfs.append(pd.DataFrame({
        'File': [row['File']],
        'Solver': ['Kececioglu'],
        'NThreads': [1],
        'Time': [row['SS_DFS_TIME(s)']],
        'M': [row['M']]
    }))

# Include rows with 'Matchmaker2' in 'INITALGO' for both 'JUST_INIT' and 'JUST_DFS'
matchmaker2_rows = filtered_df[filtered_df['INITALGO'] == 'Matchmaker2']

for _, row in matchmaker2_rows.iterrows():
    if row['PREPROCESSING'] == 'JUST_INIT':
        solver = 'Matchmaker2'
        n_threads = 'GPU'

        dfs.append(pd.DataFrame({
            'File': [row['File']],
            'Solver': [solver],
            'NThreads': [n_threads],
            'Time': [row['INIT_TIME(S)']],
            'M': [row['INIT_M']]
        }))

# Load the "empirical_results.csv" file
df_empirical = pd.read_csv("empirical_results.csv")

# Process rows for empirical_results
for _, row in df_empirical.iterrows():
    dfs.append(pd.DataFrame({
        'File': [row['File']],
        'Solver': ['Schwing'],  # Adjust this based on your requirements
        'NThreads': [row['Threads']],
        'Time': [row['MeanMatchingTime']],
        'M': [row['M']]  # Adjust this based on your requirements
    }))


# Concatenate the dataframes
result_df = pd.concat(dfs, ignore_index=True)
# Sort the DataFrame by the "File" column
result_df = result_df.sort_values(by=['File', 'NThreads'])
# Display the new dataframe
print(result_df)
