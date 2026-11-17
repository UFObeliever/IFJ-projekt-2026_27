import ifj26.*

@main
def main(): Unit =
  for i <- 1 to 6 if i < 2 || i > 5 do print(i)
