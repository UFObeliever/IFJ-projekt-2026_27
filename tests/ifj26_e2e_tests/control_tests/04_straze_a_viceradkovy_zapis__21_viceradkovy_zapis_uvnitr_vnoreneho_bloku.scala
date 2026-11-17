import ifj26.*

def run(n: Int): Unit =
  if n > 0 then
    for
      i <- 1 to n
      if i != 2
    do
      print(i)

@main
def main(): Unit =
  run(4)
