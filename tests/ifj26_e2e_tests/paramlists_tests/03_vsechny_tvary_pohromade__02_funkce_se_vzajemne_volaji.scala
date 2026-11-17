import ifj26.*

def base: Int = 5

def twice(x: Int)(y: Int): Int = x + y

def calc(): Int = twice(base)(base)

@main
def main(): Unit =
  val r = calc()
  print(r)
