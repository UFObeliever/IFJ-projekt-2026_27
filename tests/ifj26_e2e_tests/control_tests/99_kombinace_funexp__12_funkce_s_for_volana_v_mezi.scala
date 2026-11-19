import ifj26.*

def three(): Int = 3

@main
def main(): Unit =
  for i <- 1 to three() do print(i)
