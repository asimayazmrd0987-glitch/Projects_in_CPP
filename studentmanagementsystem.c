#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define PASSWORD "king1234"
#define PASS_LEN 8
// struct for student data
struct student
{
  char name[30];
  char reg[15];
  int marks[7];
  // int total_marks;
  double cgpa_all[7];
  double total_gpa;
};
const char *pass_fail2(double grade2);
const char *pass_fail(double grades);
void get_password(char arr[]);
void print_student(int index);
void clear_input();
void add_student();
void view_all();
int total_marks(int index);
void delete_all_students();
void update_student(void);
void search_student();
void name();
void map_marks(int mrk[]);
void cgpa();
void get_valid_reg(char reg[], size_t size);
void get_valid_name(char reg[], size_t size);
void show_student();
int get_valid_int(const char *prompt, int min, int max);

struct student *s = NULL;
int count = 0;
size_t max_student = 0;
// function for adding and deleting data of the students

void admin()
{
  while (1)
  {
    char pass[9];
    get_password(pass);
    if (!strcmp(pass, PASSWORD))
    {
      break;
    }
    printf("Invalid password! Try again\n");
  }
  int run2 = 1;
  while (run2)
  {
    printf("\t\t==(1)==Add Students Data====\n");
    printf("\t\t==(2)==Delete Students Data====\n");
    printf("\t\t==(3)==Update Students Data====\n");
    printf("\t\t==(4)==Quit====\n");

    int choose_1 = get_valid_int("\n\t\t====Select a choice====\n", 1, 4);

    switch (choose_1)
    {
    case 1:
      add_student();
      break;
    case 2:
      delete_all_students();
      break;
    case 3:
      update_student();
      break;
    case 4:
      run2 = 0;
      break;
    }
  }
}

void update_student(void)
{
  if (count == 0)
  {
    printf("No data found to update!\n");
    return;
  }

  char search_name[50];

  /* ---- 1. name to search ---- */
  printf("Enter student name to update:\n");
  if (!fgets(search_name, sizeof(search_name), stdin))
  {
    printf("Input error!\n");
    return;
  }
  search_name[strcspn(search_name, "\n")] = '\0';

  if (search_name[0] == '\0')
  {
    printf("Name cannot be empty!\n");
    return;
  }

  for (int i = 0; i < count; i++)
  {
    if (strcmp(search_name, s[i].name) == 0)
    {
      printf("Student found! Updating data...\n");

      get_valid_name(s[i].name, sizeof(s[i].name));

      // Update registration
      get_valid_reg(s[i].reg, sizeof(s[i].reg));

      // Update marks
      size_t j;
      for (j = 0; j < sizeof(s[i].marks) / sizeof(s[i].marks[0]); j++)
      {
        switch (j)
        {
        case 0:
          s[i].marks[j] = get_valid_int("\t\t==Programming Fundamental Theory==\n", 1, 100);
          break;
        case 1:
          s[i].marks[j] = get_valid_int("\t\t==Programming Fundamental Lab==\n", 1, 100);
          break;
        case 2:
          s[i].marks[j] = get_valid_int("\t\t==ICT Theory==\n", 1, 100);
          break;
        case 3:
          s[i].marks[j] = get_valid_int("\t\t==ICT Lab==\n", 1, 100);
          break;
        case 4:
          s[i].marks[j] = get_valid_int("\t\t==Islamic Studies==\n", 1, 100);
          break;
        case 5:
          s[i].marks[j] = get_valid_int("\t\t==Functional English==\n", 1, 100);
          break;
        case 6:
          s[i].marks[j] = get_valid_int("\t\t==PRE CALCULUS==\n", 1, 100);
          break;
        }
      }

      cgpa(i); // recompute GPA for this student only
      printf("Student data updated successfully!\n");
      return;
    }
  }

  printf("Student not found!\n");
}

void clear_input()
{
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}
// function for getting password from the user and checking the password from the user
void get_password(char arr[])
{

  char buffer[100];
  int run = 1;

  while (run)
  {
    printf("Enter 8-characters password\n");

    if (!fgets(buffer, sizeof(buffer), stdin))
    {
      printf("Error reading input\n");
      continue;
    }
    buffer[strcspn(buffer, "\n")] = '\0';

    int len = strlen(buffer);

    if (len < PASS_LEN)
    {
      printf("password too short must be 8-characters\n");
    }
    else if (len > PASS_LEN)
    {
      printf("password too long must be 8-characters\n");
    }
    else
    {
      strcpy(arr, buffer);
      run = 0;
    }
  }
}
// function for checking the user input
int get_valid_int(const char *prompt, int min, int max)
{
  char buffer[100];
  int value;

  while (1)
  {
    printf("%s", prompt);

    if (!fgets(buffer, sizeof(buffer), stdin))
    {
      printf("\n\t\tError reading input!\n\t\t\tTry again.\n");
      continue;
    }

    // Check if input was too long (no newline found)
    if (strchr(buffer, '\n') == NULL)
    {
      // Input too long: truncate at 99 chars
      buffer[99] = '\0'; // force null-termination
      // Clear remaining characters from stdin
      clear_input();
      printf("Input too long. Only first 99 characters are considered.\n");
    }
    else
    {
      // Remove newline if present
      buffer[strcspn(buffer, "\n")] = '\0';
    }

    // Remove newline character
    buffer[strcspn(buffer, "\n")] = 0;

    // Check that all characters are digits
    int valid = 1;
    for (int i = 0; buffer[i]; i++)
    {
      if (!isdigit(buffer[i]))
      {
        valid = 0;
        break;
      }
    }
    if (!valid)
    {
      printf("\n\t\tInvalid input!\n\t\t\tEnter digits only.\n");
      continue;
    }

    // Convert string to integer
    value = atoi(buffer);

    // Check limits
    if (value < min || value > max)
    {
      printf("\n\t\tValue must be between %d and %d.\n", min, max);
      continue;
    }

    return value; // valid input
  }
}
// // function to add student data
void add_student()
{
  if (count == 0)
  {
    int flag1 = 1;

    while (flag1)
    {
      max_student = get_valid_int("For How much student you want to Enter the data\n", 1, 1000);
      break;
    }
    // free old data when reallocating again
    if (s != NULL)
    {
      free(s);
      s = NULL;
      count = 0;
    }
    // memory allocation dynamically
    s = malloc(max_student * sizeof(struct student));

    if (s == NULL)
    {
      printf("Memory Allocation Failed!\n");
      return;
    }
    printf("\t\t==Enter Students Data==\n");

    size_t i;
    for (i = 0; i < max_student; i++)
    {
      printf("==Student (%d)==\n", i + 1);

      if (count >= max_student)
      {
        printf("Student Limit Reached!\n");
        break;
      }

      get_valid_name(s[i].name, sizeof(s[i].name));
      get_valid_reg(s[i].reg, sizeof(s[i].reg));

      printf("==Marks==\n");
      // each student marks
      size_t j;
      for (j = 0; j < sizeof(s[i].marks) / sizeof(s[i].marks[0]); j++)
      {
        switch (j)
        {
        case 0:
          s[i].marks[j] = get_valid_int("\t\t==Programming Fundamental Theory==\n", 1, 100);
          break;
        case 1:
          s[i].marks[j] = get_valid_int("\t\t==Programming Fundamental Lab==\n", 1, 100);
          break;
        case 2:
          s[i].marks[j] = get_valid_int("\t\t==ICT Theory==\n", 1, 100);
          break;
        case 3:
          s[i].marks[j] = get_valid_int("\t\t==ICT Lab==\n", 1, 100);
          break;
        case 4:
          s[i].marks[j] = get_valid_int("\t\t==Islamic Studies==\n", 1, 100);
          break;
        case 5:
          s[i].marks[j] = get_valid_int("\t\t==Functional English==\n", 1, 100);
          break;
        case 6:
          s[i].marks[j] = get_valid_int("\t\t==PRE CALCULUS==\n", 1, 100);
          break;
        }
      }
      count++;
    }
    count = max_student;
    cgpa();
  }
  else
  {
    printf("\nYou already enter data you cannot enter data again!\n\t\t\t\tUpdation is possible only\n");
  }
}

void get_valid_name(char name[], size_t size)
{
  while (1)
  {
    printf("Enter name of student:\n");

    if (!fgets(name, size, stdin))
    {
      printf("Reading input error!\n");
      continue;
    }

    /* If input too long */
    if (!strchr(name, '\n'))
    {
      clear_input();
      printf("Error: Name must be less than %zu characters.\n", size - 1);
      continue;
    }

    name[strcspn(name, "\n")] = '\0';

    if (name[0] == '\0')
    {
      printf("Name cannot be empty!\n");
      continue;
    }
    int valid = 1;
    for (int i = 0; name[i]; i++)
    {
      if (!isalpha((unsigned char)name[i]) && name[i] != ' ')
      {
        valid = 0;
        break;
      }
    }
    if (!valid)
    {
      printf("Invalid name! Use letters and spaces only.\n");
      continue;
    }
    return; // valid name
  }
}

void get_valid_reg(char reg[], size_t size)
{
  while (1)
  {
    printf("Enter registration number:\n");

    if (!fgets(reg, size, stdin))
    {
      printf("Reading input error!\n");
      continue;
    }

    /* Detect overflow */
    if (!strchr(reg, '\n'))
    {
      clear_input();
      printf("Error: Registration number too long.\n");
      continue;
    }

    reg[strcspn(reg, "\n")] = '\0';

    if (reg[0] == '\0')
    {
      printf("Registration cannot be empty!\n");
      continue;
    }

    int letters = 0, digits = 0, invalid = 0;

    for (int i = 0; reg[i]; i++)
    {
      if (isalpha((unsigned char)reg[i]))
        letters++;
      else if (isdigit((unsigned char)reg[i]))
        digits++;
      else
      {
        invalid = 1;
        break;
      }
    }

    if (invalid)
    {
      printf("Invalid input! Only letters and digits allowed.\n");
      continue;
    }

    if (letters == 0 || digits == 0)
    {
      printf("Registration must contain letters AND digits.\n");
      continue;
    }
    return; // valid registration
  }
}

// function for showing the paper passing and failing status
const char *pass_fail(double grade)
{
  if (grade < 0.5)
  {
    return "Fail";
  }
  else
  {
    return "Pass";
  }
}

const char *pass_fail2(double grade2)
{
  if (grade2 < 2)
  {
    return "Fail";
  }
  else
  {
    return "Pass";
  }
}

// print data
void print_student(int index)
{
  // print name
  printf("\t%s\n", s[index].name);
  // print registration number
  printf("\t%s\n", s[index].reg);
  // print marks
  for (int j = 0; j < sizeof(s[0].marks) / sizeof(s[0].marks[0]); j++)
  {
    switch (j)
    {
    case 0:
      printf("\t\tProgramming Fundamental Theory =%d \t GPA = %.1f \t %s \n", s[index].marks[j], s[index].cgpa_all[j], pass_fail(s[index].cgpa_all[j]));
      break;
    case 1:
      printf("\t\tProgramming Fundamental Lab    =%d \t GPA = %.1f \t %s \n", s[index].marks[j], s[index].cgpa_all[j], pass_fail(s[index].cgpa_all[j]));
      break;
    case 2:
      printf("\t\tICT Theory                     =%d \t GPA = %.1f \t %s \n", s[index].marks[j], s[index].cgpa_all[j], pass_fail(s[index].cgpa_all[j]));
      break;
    case 3:
      printf("\t\tICT LAB                        =%d \t GPA = %.1f \t %s \tCGPA = %.1f \t %s\n", s[index].marks[j], s[index].cgpa_all[j], pass_fail(s[index].cgpa_all[j]), s[index].total_gpa, pass_fail2(s[index].total_gpa));
      break;
    case 4:
      printf("\t\tIslamic Studies                =%d \t GPA = %.1f \t %s \n", s[index].marks[j], s[index].cgpa_all[j], pass_fail(s[index].cgpa_all[j]));
      break;
    case 5:
      printf("\t\tFunctional English             =%d \t GPA = %.1f \t %s \n", s[index].marks[j], s[index].cgpa_all[j], pass_fail(s[index].cgpa_all[j]));
      break;
    case 6:
      printf("\t\tPRE CALCULUS                   =%d \t GPA = %.1f \t %s \n", s[index].marks[j], s[index].cgpa_all[j], pass_fail(s[index].cgpa_all[j]));
      printf("\n");
      break;
    }
  }
  // print total marks
  int res2 = total_marks(index);
  printf("\t\tTotal Marks                    =%d\n", res2);
}
// function for viewing student data
void view_student()
{
  if (count == 0)
  {
    printf("Data Not Found!\n");
  }
  else
  {
    show_student();
  }
}
// // function for showing student data
void show_student()
{
  int run4 = 1;
  while (run4)
  {
    printf("\t\t==(1)==View ALL Student Data====\n");
    printf("\t\t==(2)==Search student====\n");
    printf("\t\t==(3)==Quit====\n");
    int chase = get_valid_int("\n\t\t====Select a choice====\n", 1, 3);
    switch (chase)
    {
    case 1:
      view_all();
      break;
    case 2:
      search_student();
      break;
    case 3:
      run4 = 0;
      break;
    }
  }
}

// view all student data
void view_all()
{
  for (int i = 0; i < count; i++)
  {
    print_student(i);
  }
}
// Search student by name
void search_student()
{
  int run3 = 1;
  while (run3)
  {
    printf("\t\t==(1)Search By Name====\n");
    printf("\t\t==(2)Quit====\n");

    int search = get_valid_int("\n\t\t====Select a choice====\n", 1, 2);

    switch (search)
    {
    case 1:
      name();
      break;
    case 2:
      run3 = 0;
      break;
    }
  }
}
void name()
{
  char named[100];
  while (1)
  {
    // name of each student
    printf("\nEnter name of student\n");
    if (!fgets(named, sizeof(named), stdin))
    {
      printf("Reading input error!\n");
      continue;
    }

    /* Check if input was too long */
    if (!strchr(named, '\n'))
    {
      clear_input();
      printf("Error: Name must be at most 99 characters long.\n");
      continue;
    }
    named[strcspn(named, "\n")] = '\0';

    if (named[0] == '\0')
    {
      printf("Name cannot be empty\n");
      continue;
    }
    int valid2 = 1;
    for (int j = 0; named[j]; j++)
    {
      if (!isalpha((unsigned char)named[j]) && named[j] != ' ')
      {
        printf("\n\t\tInvalid Input ! \n");
        valid2 = 0;
        break;
      }
    }
    if (!valid2)
    {
      printf("\t\t\tEnter letters only\n");
      continue;
    }
    break;
  }
  int flag = 0;
  for (int k = 0; k < count; k++)
  {
    if (!strcmp(named, s[k].name))
    {
      printf("data found!\n");
      print_student(k);
      flag = 1;
    }
  }
  if (!flag)
  {
    printf("No Name Found!\n");
  }
}

void delete_all_students()
{
  if (count == 0 || s == NULL)
  {
    printf("No data to delete!\n");
    return;
  }

  free(s);
  s = NULL;
  count = 0;
  max_student = 0;

  printf("All student data deleted successfully!\n");
}

// total marks

int total_marks(int index)
{
  int total_mrk = 0;
  for (int i = 0; i < sizeof(s[0].marks) / sizeof(s[0].marks[0]); i++)
  {
    total_mrk += s[index].marks[i];
  }
  return total_mrk;
}
// for gpa calculation
void cgpa()
{
  int count = sizeof(s[0].marks) / sizeof(s[0].marks[0]);
  int gpa_count = sizeof(s[0].cgpa_all) / sizeof(s[0].cgpa_all[0]);
  int count_student = max_student;
  int diff = 0;
  // loops for finding the highest marks
  // outer loop iterate over each subject
  for (int f = 0; f < count; f++)
  {
    int highest_marks = s[0].marks[f];
    // inner loop go through each student and get the highest marks
    for (int g = 0; g < count_student; g++)
    {
      if (s[g].marks[f] > highest_marks)
      {
        highest_marks = s[g].marks[f];
      }
    }
    if (highest_marks < 100)
    {
      diff = 100 - highest_marks;
      // loop for setting all the same highest marks to 100
      for (int g = 0; g < count_student; g++)
      {
        if (highest_marks == s[g].marks[f])
        {
          s[g].marks[f] = 100;
        }
      }
    }
    for (int g = 0; g < count_student; g++)
    {
      if (s[g].marks[f] != 100)
      {
        s[g].marks[f] += diff;
      }
    }
  }
  // now criteria of cgpa
  // outer loop for checking all the subject
  for (int k = 0; k < gpa_count; k++)
  {
    for (int m = 0; m < count_student; m++)
    {
      if (s[m].marks[k] >= 95 && s[m].marks[k] <= 100)
      {
        s[m].cgpa_all[k] = 4.0;
      }
      else if (s[m].marks[k] < 95 && s[m].marks[k] >= 90)
      {
        s[m].cgpa_all[k] = 3.5;
      }
      else if (s[m].marks[k] < 90 && s[m].marks[k] >= 85)
      {
        s[m].cgpa_all[k] = 3.0;
      }
      else if (s[m].marks[k] < 85 && s[m].marks[k] >= 80)
      {
        s[m].cgpa_all[k] = 2.5;
      }
      else if (s[m].marks[k] < 80 && s[m].marks[k] >= 75)
      {
        s[m].cgpa_all[k] = 2.0;
      }
      else if (s[m].marks[k] < 75 && s[m].marks[k] >= 70)
      {
        s[m].cgpa_all[k] = 1.5;
      }
      else if (s[m].marks[k] < 70 && s[m].marks[k] >= 65)
      {
        s[m].cgpa_all[k] = 1.0;
      }
      else if (s[m].marks[k] < 65 && s[m].marks[k] >= 60)
      {
        s[m].cgpa_all[k] = 0.5;
      }
      else
      {
        s[m].cgpa_all[k] = 0.0;
      }
    }
  }

  // now totaling the gpa
  for (int i = 0; i < count_student; i++)
  {
    double tot_gpa = 0;
    for (int j = 0; j < gpa_count; j++)
    {
      tot_gpa += s[i].cgpa_all[j];
    }
    s[i].total_gpa = tot_gpa / 7;
  }
}

// main function
int main()
{
  int choice = 0;
  char input[3];
  // for getting the user choice
  int running = 1;
  while (running)
  {
    printf("\t\t=======Menu====\n");
    printf("\t\t==(1)==View Student====\n");
    printf("\t\t==(2)==Admin====\n");
    printf("\t\t==(3)==Exit====\n");
    printf("\n\t\t==Select a Choice====\n");

    if (!fgets(input, sizeof(input), stdin))
    {
      printf("Error reading input\n");
      continue;
    }
    if (input[strlen(input) - 1] != '\n')
    {
      clear_input();
    }
    int valid = 1;
    input[strcspn(input, "\n")] = '\0';
    for (int i = 0; input[i]; i++)
    {
      if (!isdigit(input[i]))
      {
        valid = 0;
        break;
      }
    }
    if (!valid)
    {
      printf("Invalid Input! try again\n");
      continue;
    }

    choice = atoi(input);

    if (choice < 1 || choice > 3)
    {
      printf("Invalid Input! Try Again\n");
      continue;
    }
    switch (choice)
    {
    case 1:
      view_student();
      break;
    case 2:
      admin();
      break;
    case 3:
      printf("==Good Bye==\n");
      running = 0;
      break;
    }
  }

  return 0;
}