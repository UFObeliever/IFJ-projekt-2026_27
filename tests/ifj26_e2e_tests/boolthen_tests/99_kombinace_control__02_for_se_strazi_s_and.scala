import ifj26.*

@main
def main(): Unit =
  for i <- 1 to 10 if i > 3 && i < 7 do print(i)
