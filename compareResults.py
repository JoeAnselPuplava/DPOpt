# def extract_flagged_rows(filename):
#     """Extract rows ending with 'Flag: 1' from a file."""
#     flagged_rows = set()
#     with open(filename, 'r') as f:
#         for line in f:
#             line = line.strip()
#             if line.endswith("| Flag: 1"):
#                 # Only keep the part before the flag
#                 row = line.split("|")[0].strip()
#                 flagged_rows.add(row)
#     return flagged_rows


# def compare_files(file1, file2):
#     flagged1 = extract_flagged_rows(file1)
#     flagged2 = extract_flagged_rows(file2)

#     # Compare sets
#     if flagged1 == flagged2:
#         print("The two files have the same Flag: 1 rows.")
#     else:
#         print("The two files differ in Flag: 1 rows.")
#         only_in_file1 = flagged1 - flagged2
#         only_in_file2 = flagged2 - flagged1

#         if only_in_file1:
#             print(f"\nRows only in {file1}:")
#             for row in only_in_file1:
#                 print(row)

#         if only_in_file2:
#             print(f"\nRows only in {file2}:")
#             for row in only_in_file2:
#                 print(row)


# # Example usage
# if __name__ == "__main__":
#     compare_files("template4Test_output", "template4Test_sara_output")

def extract_flagged_rows_from_sections(filename):
    """Extract flagged rows separately for Implementation A and B from one file."""
    flagged_A = set()
    flagged_B = set()
    current_section = None

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()

            # Detect section headers
            if line.startswith("Implementation A (FilterOperator) Result:"):
                current_section = "A"
                continue
            elif line.startswith("Implementation B (FilterOperatorSyscat + planNode) Result:"):
                current_section = "B"
                continue

            # Capture flagged rows only within known sections
            if current_section and line.endswith("| Flag: 1"):
                row = line.split("|")[0].strip()
                if current_section == "A":
                    flagged_A.add(row)
                elif current_section == "B":
                    flagged_B.add(row)

    return flagged_A, flagged_B


def compare_sections(filename):
    flagged_A, flagged_B = extract_flagged_rows_from_sections(filename)

    if flagged_A == flagged_B:
        print("Implementation A and B have the same Flag: 1 rows.")
    else:
        print("Implementation A and B differ in Flag: 1 rows.")

        only_in_A = flagged_A - flagged_B
        only_in_B = flagged_B - flagged_A

        if only_in_A:
            print("\nRows only in Implementation A:")
            for row in only_in_A:
                print(row)

        if only_in_B:
            print("\nRows only in Implementation B:")
            for row in only_in_B:
                print(row)


# Example usage
if __name__ == "__main__":
    compare_sections("template2Test")
