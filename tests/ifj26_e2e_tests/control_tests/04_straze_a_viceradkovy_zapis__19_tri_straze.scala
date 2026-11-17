import ifj26.*

@main
def main(): Unit =
  for
    i <- 1 to 20
    if i > 5
    if i < 15
    if i / 3 * 3 == i
  do
    print(i)
    print(" ")
