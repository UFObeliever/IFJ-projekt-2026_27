import ifj26.*

def pick(c: Boolean)(a: Int)(b: Int): Int = if c then a else b

@main
def main(): Unit =
  val r = pick(false)(1)(2)
  print(r)
