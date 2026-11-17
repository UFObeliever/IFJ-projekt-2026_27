import ifj26.*

def count(n: Int): Unit =
  for i <- 1 to n do
    print(i)

@main
def main(): Unit =
  count(3)
