import ifj26.*

extension (v: Int)
  def noisy: Int =
    print("n")
    v

@main
def main(): Unit =
  val x = 1
  val a = x.noisy
  val b = x.noisy
  val c = x.noisy
  print(a + b + c)
