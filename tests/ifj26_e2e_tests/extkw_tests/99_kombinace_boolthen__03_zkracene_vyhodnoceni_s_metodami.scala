import ifj26.*

extension (n: Int)
  def noisy: Boolean =
    print("n")
    n > 0

@main
def main(): Unit =
  val a = 0 - 1
  val b = 1
  val r = a.noisy && b.noisy
  print("|")
  print(r)
