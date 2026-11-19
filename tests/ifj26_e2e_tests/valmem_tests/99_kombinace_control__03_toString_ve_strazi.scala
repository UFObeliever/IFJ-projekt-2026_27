import ifj26.*

@main
def main(): Unit =
  for i <- 1 until 15 if i.toString.length == 1 do print(i)
