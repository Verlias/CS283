#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file



@test "Basic: ls should run without errors" {
    run ./dsh <<EOF
ls
EOF
    [ "$status" -eq 0 ]
}

@test "Basic: pwd should return current directory" {
    run ./dsh <<EOF
pwd
EOF
    [ "$status" -eq 0 ]
    [ -n "$output" ]  # Ensure output is not empty
}



@test "Built-in: exit should terminate shell" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Built-in: cd should change directory" {
    run ./dsh <<EOF
cd /
pwd
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"/"* ]]
}


@test "Piping: ls | wc -l should count files" {
    run ./dsh <<EOF
ls | wc -l
EOF
    [ "$status" -eq 0 ]
    [ -n "$output" ]  # Ensure output is not empty
}

@test "Redirection: echo 'Hello' > testfile" {
    run ./dsh <<EOF
echo "Hello" > testfile
EOF
    [ "$status" -eq 0 ]
    [ -f "testfile" ]  # Check if file was created
    [ "$(cat testfile)" = "Hello" ]
    rm testfile  # Cleanup
}


@test "Custom Command: dragon should print ASCII art" {
    run ./dsh <<EOF
dragon
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"%%%%"* ]]  # Check if output contains expected ASCII pattern
}