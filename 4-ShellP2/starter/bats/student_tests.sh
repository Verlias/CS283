#!/usr/bin/env bats

# File: dsh_tests.bats
# Test suite for the dsh shell implementation

# Ensure dsh is compiled before running tests
setup() {
    gcc dsh_cli.c dshlib.c -o dsh
    chmod +x dsh
}

@test "Check if dsh exists and is executable" {
    [ -f "./dsh" ]
    [ -x "./dsh" ]
}

@test "Basic command execution: ls" {
    run ./dsh <<EOF
ls
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh"* ]]  # Check if dsh itself appears in ls output
}

@test "Built-in command: exit" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Built-in command: cd" {
    run ./dsh <<EOF
cd /
pwd
EOF
    [[ "$output" == *"/"* ]]
}

@test "Execute external command: echo" {
    run ./dsh <<EOF
echo Hello, world!
EOF
    [[ "$output" == *"Hello, world!"* ]]
}

@test "Handle quoted spaces in arguments" {
    run ./dsh <<EOF
echo "Hello   World"
EOF
    [[ "$output" == *"Hello   World"* ]]
}

@test "Handle multiple spaces between arguments" {
    run ./dsh <<EOF
echo    spaced    out
EOF
    [[ "$output" == *"spaced out"* ]]
}


@test "Execute command with arguments" {
    run ./dsh <<EOF
ls -l
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"total"* ]]
}


@test "Multiple commands in sequence" {
    run ./dsh <<EOF
echo first
echo second
EOF
    [[ "$output" == *"first"* ]]
    [[ "$output" == *"second"* ]]
}


