#!/usr/bin/env bats
# File: student_tests.sh
# Updated unit test suite for your dsh shell

# Test that ls runs without errors.
@test "ls runs without errors" {
  run ./dsh <<EOF
ls
exit
EOF
  [ "$status" -eq 0 ]
  # Optionally, you might check for expected output (e.g., prompt or file names)
}

# Test that pwd returns the current working directory.
@test "pwd returns current working directory" {
  run ./dsh <<EOF
pwd
exit
EOF
  expected="$(pwd)"
  # Assuming that the first line of output is the result of pwd.
  actual="$(echo "$output" | head -n 1 | tr -d '\r\n')"
  [ "$status" -eq 0 ]
  [ "$actual" = "$expected" ]
}

# Test that echo prints the correct message.
@test "echo prints correct message" {
  run ./dsh <<EOF
echo "Hello, world!"
exit
EOF
  expected="Hello, world!"
  actual="$(echo "$output" | head -n 1 | tr -d '\r\n')"
  [ "$status" -eq 0 ]
  [ "$actual" = "$expected" ]
}

# Test that the exit command terminates the shell.
@test "exit command terminates the shell" {
  run ./dsh <<EOF
exit
EOF
  [ "$status" -eq 0 ]
}

# Test that basic piping works (e.g., ls | wc -l returns a number).
@test "basic piping works" {
  run ./dsh <<EOF
ls | wc -l
exit
EOF
  trimmed="$(echo "$output" | head -n 1 | tr -d '[:space:]')"
  [ "$status" -eq 0 ]
  [[ "$trimmed" =~ ^[0-9]+$ ]]
}


@test "Check cd changes directory" {
    run ./dsh <<EOF
cd ..
pwd
EOF
    expected_output="$(cd .. && pwd)"
    actual_output="$(echo "$output" | grep -Eo '/mnt/c/Users/logan/OneDrive/Desktop/CS283/CS283')"
    echo "Expected: $expected_output"
    echo "Actual: $actual_output"
    [ "$status" -eq 0 ]
    [[ "$actual_output" == "$expected_output" ]]
}
