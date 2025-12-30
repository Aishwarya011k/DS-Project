#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 10

/* ---------------- PRODUCT STRUCTURE (HASH TABLE) ---------------- */
struct Product {
    int id;
    char name[50];
    float price;
    int stock;
    struct Product *next;
};

struct Product *hashTable[TABLE_SIZE];

/* ---------------- CART STRUCTURE (LINKED LIST) ---------------- */
struct CartItem {
    int productId;  
    char name[50];
    float price;   
    int quantity;    
    struct CartItem *next;    
};

struct CartItem *cartHead = NULL;

/* ---------------- HASH FUNCTION ---------------- */
int hashFunction(int id) {
    return id % TABLE_SIZE;
}

/* ---------------- ADD PRODUCT ---------------- */
void addProduct(int id, char name[], float price, int stock) {
    int index = hashFunction(id);
    struct Product *p = malloc(sizeof(struct Product));

    p->id = id;
    strcpy(p->name, name);
    p->price = price;
    p->stock = stock;
    p->next = hashTable[index];

    hashTable[index] = p;
}

/* ---------------- SEARCH PRODUCT ---------------- */
struct Product* searchProduct(int id) {
    int index = hashFunction(id);
    struct Product *temp = hashTable[index];

    while (temp) {
        if (temp->id == id)
            return temp;
        temp = temp->next;
    }
    return NULL;
}

/* ---------------- DISPLAY PRODUCTS ---------------- */
void displayProducts() {
    printf("\n--- Available Products ---\n");
    for (int i = 0; i < TABLE_SIZE; i++) {
        struct Product *temp = hashTable[i];
        while (temp) {
            printf("ID: %d | Name: %s | Price: %.2f | Stock: %d\n",
                   temp->id, temp->name, temp->price, temp->stock);
            temp = temp->next;
        }
    }
}

/* ---------------- ADD TO CART ---------------- */
void addToCart(int productId, int qty) {
    struct Product *p = searchProduct(productId);
    if (!p) {
        printf("Product not found!\n");
        return;
    }

    if (qty > p->stock) {
        printf("Insufficient stock!\n");
        return;
    }

    struct CartItem *temp = cartHead;
    while (temp) {
        if (temp->productId == productId) {
            if (temp->quantity + qty > p->stock) {
                printf("Cannot exceed available stock!\n");
                return;
            }
            temp->quantity += qty;
            printf("Cart updated successfully.\n");
            return;
        }
        temp = temp->next;
    }

    struct CartItem *newItem = malloc(sizeof(struct CartItem));
    newItem->productId = p->id;
    strcpy(newItem->name, p->name);
    newItem->price = p->price;
    newItem->quantity = qty;
    newItem->next = cartHead;

    cartHead = newItem;
    printf("Item added to cart.\n");
}

/* ---------------- REMOVE FROM CART ---------------- */
void removeFromCart(int productId) {
    struct CartItem *temp = cartHead, *prev = NULL;

    while (temp) {
        if (temp->productId == productId) {
            if (prev)
                prev->next = temp->next;
            else
                cartHead = temp->next;
            free(temp);
            printf("Item removed from cart.\n");
            return;
        }
        prev = temp;
        temp = temp->next;
    }
    printf("Item not found in cart.\n");
}

/* ---------------- UPDATE CART QUANTITY ---------------- */
void updateCartQuantity(int productId, int newQty) {
    struct CartItem *temp = cartHead, *prev = NULL;

    while (temp) {
        if (temp->productId == productId) {
            struct Product *p = searchProduct(productId);

            if (newQty <= 0) {
                if (prev)
                    prev->next = temp->next;
                else
                    cartHead = temp->next;
                free(temp);
                printf("Item removed (quantity zero).\n");
            }
            else if (newQty > p->stock) {
                printf("Insufficient stock!\n");
            }
            else {
                temp->quantity = newQty;
                printf("Quantity updated.\n");
            }
            return;
        }
        prev = temp;
        temp = temp->next;
    }
    printf("Item not found in cart.\n");
}

/* ---------------- DISPLAY CART ---------------- */
void displayCart() {
    if (!cartHead) {
        printf("Cart is empty.\n");
        return;
    }

    struct CartItem *temp = cartHead;
    float total = 0;

    printf("\n--- Shopping Cart ---\n");
    while (temp) {
        float cost = temp->price * temp->quantity;
        printf("%s | Qty: %d | Price: %.2f | Cost: %.2f\n",
               temp->name, temp->quantity, temp->price, cost);
        total += cost;
        temp = temp->next;
    }
    printf("Total Amount: %.2f\n", total);
}

/* ---------------- CLEAR CART ---------------- */
void clearCart() {
    struct CartItem *temp;
    while (cartHead) {
        temp = cartHead;
        cartHead = cartHead->next;
        free(temp);
    }
    printf("Cart cleared successfully.\n");
}

/* ---------------- COUNT CART ITEMS ---------------- */
int cartItemCount() {
    int count = 0;
    struct CartItem *temp = cartHead;
    while (temp) {
        count += temp->quantity;
        temp = temp->next;
    }
    return count;
}

/* ---------------- CHECKOUT ---------------- */
void checkout() {
    if (!cartHead) {
        printf("Cart is empty!\n");
        return;
    }

    struct CartItem *temp = cartHead;
    float total = 0;

    printf("\n--- Checkout Bill ---\n");
    while (temp) {
        struct Product *p = searchProduct(temp->productId);
        if (p) {
            p->stock -= temp->quantity;
            total += temp->price * temp->quantity;
        }
        temp = temp->next;
    }

    printf("Final Amount Payable: %.2f\n", total);
    clearCart();
    printf("Thank you for shopping!\n");
}

/* ---------------- MAIN FUNCTION ---------------- */
int main() {
    int choice, id, qty;

    /* Sample Products */
    addProduct(101, "Laptop", 50000, 5);
    addProduct(102, "Phone", 20000, 10);
    addProduct(103, "Headphones", 2000, 15);
    addProduct(104, "Keyboard", 1500, 20);

    while (1) {
        printf("\n========= E-COMMERCE MENU =========\n");
        printf("1. Display Products\n");
        printf("2. Add to Cart\n");
        printf("3. Remove from Cart\n");
        printf("4. Update Cart Quantity\n");
        printf("5. View Cart\n");
        printf("6. Clear Cart\n");
        printf("7. Cart Item Count\n");
        printf("8. Checkout\n");
        printf("9. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
        case 1:
            displayProducts();
            break;
        case 2:
            printf("Enter Product ID and Quantity: ");
            scanf("%d %d", &id, &qty);
            addToCart(id, qty);
            break;
        case 3:
            printf("Enter Product ID: ");
            scanf("%d", &id);
            removeFromCart(id);
            break;
        case 4:
            printf("Enter Product ID and New Quantity: ");
            scanf("%d %d", &id, &qty);
            updateCartQuantity(id, qty);
            break;
        case 5:
            displayCart();
            break;
        case 6:
            clearCart();
            break;
        case 7:
            printf("Total items in cart: %d\n", cartItemCount());
            break;
        case 8:
            checkout();
            break;
        case 9:
            exit(0);
        default:
            printf("Invalid choice!\n");
        }
    }
}
