import ifj26.*

def offset()(value: Int): Int = value + 1

@main
def main(): Unit =
  val r = offset()(5)
  print(r)
