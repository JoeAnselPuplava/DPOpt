import numpy as np
import pandas as pd
import argparse

def generate_normal_csv(filename, num_rows=16, num_cols=3, mean=50, stddev=10):
    """
    Generates a CSV with normally-distributed integer data.
    
    Args:
        filename (str): The name of the output CSV file.
        num_rows (int): Number of rows of data to generate.
        num_cols (int): Number of columns of data.
        mean (float): Mean of the normal distribution.
        stddev (float): Standard deviation of the normal distribution.
    """
    # Create normally-distributed float data
    data = np.random.normal(loc=mean, scale=stddev, size=(num_rows, num_cols))

    # Convert to integers (EMP expects Integer()s)
    data = np.round(data).astype(int)

    # Save as CSV
    df = pd.DataFrame(data, columns=[f"col{i}" for i in range(num_cols)])
    df.to_csv(filename, index=False)
    print(f"Generated {num_rows} rows and {num_cols} columns to {filename}.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default="normal_data.csv", help="Output CSV file name")
    parser.add_argument("--rows", type=int, default=16, help="Number of rows")
    parser.add_argument("--cols", type=int, default=3, help="Number of columns")
    parser.add_argument("--mean", type=float, default=50.0, help="Mean of the normal distribution")
    parser.add_argument("--std", type=float, default=10.0, help="Standard deviation")
    args = parser.parse_args()

    generate_normal_csv(args.output, args.rows, args.cols, args.mean, args.std)
