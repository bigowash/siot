def analyze_log_data(file_path):
    import re
    from collections import defaultdict

    # Regular expression to match lines with operation and time taken
    regex = re.compile(r'(\w[\w\s]*) took (\d+) ms')

    # Dictionary to hold total time and count for each operation
    operation_data = defaultdict(lambda: {'total_time': 0, 'count': 0})

    with open(file_path, 'r') as file:
        for line in file:
            match = regex.search(line)
            if match:
                operation, time = match.groups()
                operation_data[operation.strip()]['total_time'] += int(time)
                operation_data[operation.strip()]['count'] += 1

    # Calculating averages
    averages = {op: data['total_time'] / data['count'] for op, data in operation_data.items()}

    return averages, operation_data

# Specify the path to your log file
file_path = "dataFiles/latency_analysis.txt"

average_durations, operation_counts = analyze_log_data(file_path)

# Pretty print the results
for operation, average in average_durations.items():
    count = operation_counts[operation]['count']
    print(f"{operation} - Average Duration: {average:.2f} ms, Count: {count}")
