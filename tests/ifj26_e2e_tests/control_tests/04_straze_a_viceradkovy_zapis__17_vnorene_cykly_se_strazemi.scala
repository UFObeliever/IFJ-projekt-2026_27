import ifj26.*

@main
def main(): Unit =
  for i <- 1 to 3 if i != 2 do
    for j <- 1 to 3 if j != i do
      print(i)
      print(j)
      print(" ")
